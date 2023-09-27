// Copyright(c) 2020-2023, Intel Corporation
//
// Redistribution  and  use  in source  and  binary  forms,  with  or  without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of  source code  must retain the  above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// * Neither the name  of Intel Corporation  nor the names of its contributors
//   may be used to  endorse or promote  products derived  from this  software
//   without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO,  THE
// IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT  SHALL THE COPYRIGHT OWNER  OR CONTRIBUTORS BE
// LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
// CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT LIMITED  TO,  PROCUREMENT  OF
// SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  DATA, OR PROFITS;  OR BUSINESS
// INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY  OF LIABILITY,  WHETHER IN
// CONTRACT,  STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE  OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif // _GNU_SOURCE
#include <byteswap.h>
#include <linux/limits.h>
#include <errno.h>
#include <glob.h>
#include <regex.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <uuid/uuid.h>
#include <sys/stat.h>
#include <sys/eventfd.h>
#include <unistd.h>
#undef _GNU_SOURCE

#include <opae/fpga.h>

#include "opae_vfio.h"
#include "dfl.h"
#include "fpga-dfl.h"

#include "opae_int.h"
#include "props.h"
#include "cfg-file.h"
#include "mock/opae_std.h"

#define VFIO_TOKEN_MAGIC 0xEF1010FE
#define VFIO_HANDLE_MAGIC ~VFIO_TOKEN_MAGIC
#define VFIO_EVENT_HANDLE_MAGIC 0x5a6446a5

#define FPGA_BBS_VER_MAJOR(i) (((i) >> 56) & 0xf)
#define FPGA_BBS_VER_MINOR(i) (((i) >> 52) & 0xf)
#define FPGA_BBS_VER_PATCH(i) (((i) >> 48) & 0xf)

#define PCIE_PATH_PATTERN "([0-9a-fA-F]{4}):([0-9a-fA-F]{2}):([0-9a-fA-F]{2})\\.([0-9])"
#define PCIE_PATH_PATTERN_GROUPS 5
#define PARSE_MATCH_INT(_p, _m, _v, _b, _l)                                    \
	do {                                                                   \
		const char *ptr = _p + _m.rm_so;                               \
		char *endptr = NULL;                                           \
		_v = strtoul(ptr, &endptr, _b);                                \
		if (endptr == ptr) {                                           \
			OPAE_ERR("error parsing int");                         \
			goto _l;                                               \
		}                                                              \
	} while (0)


const char *fme_drivers[] = {
	"bfaf2ae9-4a52-46e3-82fe-38f0f9e17764",
	0
};

STATIC vfio_pci_device_t *_pci_devices;

STATIC int read_file(const char *path, char *value, size_t max)
{
	int res = FPGA_OK;
	FILE *fp;

	fp = opae_fopen(path, "r");
	if (!fp) {
		OPAE_ERR("error opening: %s", path);
		return FPGA_EXCEPTION;
	}

	if (!fread(value, 1, max, fp)) {
		OPAE_ERR("error reading from: %s", path);
		res = FPGA_EXCEPTION;
	}

	opae_fclose(fp);
	return res;
}

STATIC int read_pci_link(const char *addr, const char *link, char *value, size_t max)
{
	char path[PATH_MAX];
	char fullpath[PATH_MAX];

	snprintf(path, sizeof(path), "/sys/bus/pci/devices/%s/%s", addr, link);
	if (!realpath(path, fullpath)) {
		if (errno == ENOENT)
			return 1;
		OPAE_ERR("error reading path: %s", path);
		return 2;
	}
	char *p = strrchr(fullpath, '/');

	if (!p) {
		OPAE_ERR("error finding '/' in path: %s", fullpath);
		return 2;
	}
	strncpy(value, p+1, max);
	return 0;
}

STATIC int read_pci_attr(const char *addr, const char *attr, char *value, size_t max)
{
	char path[PATH_MAX];

	snprintf(path, sizeof(path),
		 "/sys/bus/pci/devices/%s/%s", addr, attr);

	return read_file(path, value, max);
}

STATIC int read_pci_attr_u32(const char *addr, const char *attr, uint32_t *value)
{
	char str_value[64] = { 0, };
	char *endptr = NULL;
	int res;
	uint32_t v;
	size_t len;

	res = read_pci_attr(addr, attr, str_value, sizeof(str_value));
	if (res)
		return res;

	len = strnlen(str_value, sizeof(str_value) - 1);
	if (str_value[len-1] == '\n') {
		--len;
		str_value[len] = '\0';
	}

	v = strtoul(str_value, &endptr, 0);

	if (endptr != &str_value[len]) {
		OPAE_ERR("error parsing string: %s", str_value);
		return FPGA_EXCEPTION;
	}

	*value = v;
	return FPGA_OK;
}

STATIC int parse_pcie_info(vfio_pci_device_t *device, const char *addr)
{
	char err[128] = { 0, };
	regex_t re;
	regmatch_t matches[PCIE_PATH_PATTERN_GROUPS] = { {0} };
	int res = FPGA_EXCEPTION;

	int reg_res = regcomp(&re, PCIE_PATH_PATTERN, REG_EXTENDED | REG_ICASE);

	if (reg_res) {
		OPAE_ERR("Error compiling regex");
		return FPGA_EXCEPTION;
	}
	reg_res = regexec(&re, addr, PCIE_PATH_PATTERN_GROUPS, matches, 0);
	if (reg_res) {
		regerror(reg_res, &re, err, 128);
		OPAE_ERR("Error executing regex: %s", err);
		res = FPGA_EXCEPTION;
		goto out;
	} else {
		PARSE_MATCH_INT(addr, matches[1], device->bdf.segment, 16, out);
		PARSE_MATCH_INT(addr, matches[2], device->bdf.bus, 16, out);
		PARSE_MATCH_INT(addr, matches[3], device->bdf.device, 16, out);
		PARSE_MATCH_INT(addr, matches[4], device->bdf.function, 10, out);
	}
	res = FPGA_OK;

out:
	regfree(&re);
	return res;
}

STATIC void free_token_list(vfio_token *tokens)
{
	while (tokens) {
		vfio_token *trash = tokens;
		tokens = tokens->next;
		opae_free(trash);
	}
}

void vfio_free_device_list(void)
{
	while (_pci_devices) {
		vfio_pci_device_t *trash = _pci_devices;
		_pci_devices = _pci_devices->next;
		free_token_list(trash->tokens);
		opae_free(trash);
	}
}

STATIC vfio_pci_device_t *find_pci_device(const char addr[PCIADDR_MAX])
{
	vfio_pci_device_t *p = _pci_devices;

	while (p) {
		if (!strncmp(p->addr, addr, PCIADDR_MAX))
			return p;
		p = p->next;
	}

	return NULL;
}

STATIC vfio_pci_device_t *vfio_get_pci_device(const char addr[PCIADDR_MAX])
{
	vfio_pci_device_t *dev;
	uint32_t svid_sdid[2] = { 0, 0 };

	dev = find_pci_device(addr);
	if (dev)
		return dev;

	dev = (vfio_pci_device_t *)opae_calloc(1, sizeof(vfio_pci_device_t));
	if (!dev) {
		OPAE_ERR("Failed to allocate memory for vfio_pci_device_t");
		return NULL;
	}

	strncpy(dev->addr, addr, PCIADDR_MAX-1);

	if (read_pci_attr_u32(addr, "vendor", &dev->vendor) ||
	    read_pci_attr_u32(addr, "device", &dev->device) ||
	    read_pci_attr_u32(addr, "subsystem_vendor", &svid_sdid[0]) ||
	    read_pci_attr_u32(addr, "subsystem_device", &svid_sdid[1])) {
		OPAE_ERR("reading PCI attributes for %s", addr);
		goto free;
	}
	dev->subsystem_vendor = (uint16_t)svid_sdid[0];
	dev->subsystem_device = (uint16_t)svid_sdid[1];

	if (read_pci_attr_u32(addr, "numa_node", &dev->numa_node)) {
		OPAE_DBG("reading numa_node for %s", addr);
		dev->numa_node = INVALID_NUMA_NODE;
	}

	if (parse_pcie_info(dev, addr)) {
		OPAE_ERR("error parsing pcie address: %s", addr);
		goto free;
	}

	dev->next = _pci_devices;
	_pci_devices = dev;
	return dev;

free:
	opae_free(dev);
	return NULL;
}

libopae_config_data *opae_v_supported_devices;

STATIC bool pci_device_matches(const libopae_config_data *c,
			       uint16_t vid,
			       uint16_t did,
			       uint16_t svid,
			       uint16_t sdid)
{
	if (strcmp(c->module_library, "libopae-v.so"))
		return false;

	if ((c->vendor_id != vid) ||
	    (c->device_id != did))
		return false;

	if ((c->subsystem_vendor_id != OPAE_VENDOR_ANY) &&
	    (c->subsystem_vendor_id != svid))
		return false;

	if ((c->subsystem_device_id != OPAE_DEVICE_ANY) &&
	    (c->subsystem_device_id != sdid))
		return false;

	return true;
}

STATIC bool pci_device_supported(const char *pcie_addr)
{
	uint32_t vendor = 0;
	uint32_t device = 0;
	uint32_t subsystem_vendor = 0;
	uint32_t subsystem_device = 0;
	size_t i;

	if (read_pci_attr_u32(pcie_addr, "vendor", &vendor) ||
	    read_pci_attr_u32(pcie_addr, "device", &device) ||
	    read_pci_attr_u32(pcie_addr, "subsystem_vendor", &subsystem_vendor) ||
	    read_pci_attr_u32(pcie_addr, "subsystem_device", &subsystem_device)) {
		OPAE_ERR("couldn't determine VID/DID SVID/SDID for %s", pcie_addr);
		return false;
	}

	for (i = 0 ; opae_v_supported_devices[i].module_library ; ++i) {
		if (pci_device_matches(&opae_v_supported_devices[i],
				       (uint16_t)vendor, (uint16_t)device,
				       (uint16_t)subsystem_vendor,
				       (uint16_t)subsystem_device))
			return true;
	}

	return false;
}

int vfio_pci_discover(const char *gpattern)
{
	int res = 1;
	glob_t pg;
	int gres;

	if (!gpattern)
		gpattern = "/sys/bus/pci/drivers/vfio-pci/????:??:??.?";

	gres = opae_glob(gpattern, 0, NULL, &pg);

	if (gres) {
		OPAE_DBG("vfio-pci not bound to any PCIe endpoint");
		res = 0;
		goto free;
	}
	if (!pg.gl_pathc) {
		goto free;
	}

	for (size_t i = 0; i < pg.gl_pathc; ++i) {
		char *p = strrchr(pg.gl_pathv[i], '/');

		if (!p) {
			OPAE_ERR("error with gl_pathv");
			continue;
		}

		if (!pci_device_supported(p + 1))
			continue;

		vfio_pci_device_t *dev = vfio_get_pci_device(p + 1);

		if (!dev) {
			OPAE_ERR("error with pci address: %s", p + 1);
		} else {
			res = 0;
		}
	}

free:
	opae_globfree(&pg);
	return res;
}

STATIC vfio_token *clone_token(vfio_token *src)
{
	vfio_token *token;

	ASSERT_NOT_NULL_RESULT(src, NULL);
	if (src->hdr.magic != VFIO_TOKEN_MAGIC)
		return NULL;

	token = (vfio_token *)opae_malloc(sizeof(vfio_token));

	if (!token) {
		OPAE_ERR("Failed to allocate memory for vfio_token");
		return NULL;
	}

	memcpy(token, src, sizeof(vfio_token));

	if (src->parent)
		token->parent = clone_token(src->parent);

	token->next = NULL;

	return token;
}

STATIC vfio_token *token_check(fpga_token token)
{
	vfio_token *t;

	ASSERT_NOT_NULL_RESULT(token, NULL);

	t = (vfio_token *)token;
	if (t->hdr.magic != VFIO_TOKEN_MAGIC) {
		OPAE_ERR("invalid token magic");
		return NULL;
	}

	return t;
}

STATIC vfio_handle *handle_check(fpga_handle handle)
{
	vfio_handle *h;

	ASSERT_NOT_NULL_RESULT(handle, NULL);

	h = (vfio_handle *)handle;
	if (h->magic != VFIO_HANDLE_MAGIC) {
		OPAE_ERR("invalid handle magic");
		return NULL;
	}

	return h;
}

STATIC vfio_event_handle *event_handle_check(fpga_event_handle event_handle)
{
	vfio_event_handle *eh;

	ASSERT_NOT_NULL_RESULT(event_handle, NULL);

	eh = (vfio_event_handle *)event_handle;
	if (eh->magic != VFIO_EVENT_HANDLE_MAGIC) {
		OPAE_ERR("invalid event handle magic");
		return NULL;
	}

	return eh;
}

STATIC vfio_handle *handle_check_and_lock(fpga_handle handle)
{
	int res;
	vfio_handle *h;

	h = handle_check(handle);
	if (h)
		return opae_mutex_lock(res, &h->lock) ? NULL : h;

	return NULL;
}

STATIC vfio_event_handle *
event_handle_check_and_lock(fpga_event_handle event_handle)
{
	int res;
	vfio_event_handle *eh;

	eh = event_handle_check(event_handle);
	if (eh)
		return opae_mutex_lock(res, &eh->lock) ? NULL : eh;

	return NULL;
}

STATIC int close_vfio_pair(vfio_pair_t **pair)
{
	ASSERT_NOT_NULL(pair);
	ASSERT_NOT_NULL(*pair);
	vfio_pair_t *ptr = *pair;

	if (ptr->device) {
		opae_vfio_close(ptr->device);
		opae_free(ptr->device);
	}
	if (ptr->physfn) {
		opae_vfio_close(ptr->physfn);
		opae_free(ptr->physfn);
	}
	opae_free(ptr);
	*pair = NULL;
	return 0;
}

STATIC fpga_result open_vfio_pair(const char *addr, vfio_pair_t **ppair)
{
	char phys_device[PCIADDR_MAX];
	char phys_driver[PATH_MAX];
	char secret[GUIDSTR_MAX];
	vfio_pair_t *pair;
	int ires;
	fpga_result res = FPGA_EXCEPTION;

	*ppair = opae_malloc(sizeof(vfio_pair_t));

	if (!*ppair) {
		OPAE_ERR("Failed to allocate memory for vfio_pair_t");
		return FPGA_NO_MEMORY;
	}

	pair = *ppair;
	memset(pair, 0, sizeof(vfio_pair_t));

	pair->device = opae_malloc(sizeof(struct opae_vfio));
	if (!pair->device) {
		OPAE_ERR("Failed to allocate memory for opae_vfio struct");
		opae_free(pair);
		*ppair = NULL;
		return FPGA_NO_MEMORY;
	}
	memset(pair->device, 0, sizeof(struct opae_vfio));

	memset(phys_device, 0, sizeof(phys_device));
	memset(phys_driver, 0, sizeof(phys_driver));
	if (!read_pci_link(addr, "physfn", phys_device, PCIADDR_MAX-1) &&
	    !read_pci_link(phys_device, "driver", phys_driver,
				PATH_MAX-1) &&
	    strstr(phys_driver, "vfio-pci")) {
		uuid_generate(pair->secret);
		uuid_unparse(pair->secret, secret);

		pair->physfn = opae_malloc(sizeof(struct opae_vfio));
		if (!pair->physfn) {
			OPAE_ERR("Failed to allocate memory for opae_vfio");
			goto out_destroy;
		}
		memset(pair->physfn, 0, sizeof(struct opae_vfio));

		ires = opae_vfio_secure_open(pair->physfn, phys_device, secret);
		if (ires) {
			if (ires == 2)
				res = FPGA_BUSY;
			else
				OPAE_ERR("error opening physfn: %s", phys_device);
			opae_free(pair->physfn);
			pair->physfn = NULL;
			goto out_destroy;
		}

		ires = opae_vfio_secure_open(pair->device, addr, secret);
		if (ires) {
			if (ires == 2)
				res = FPGA_BUSY;
			else
				OPAE_ERR("error opening vfio device: %s", addr);
			opae_free(pair->physfn);
			pair->physfn = NULL;
			goto out_destroy;
		}
	} else {
		ires = opae_vfio_open(pair->device, addr);
		if (ires) {
			if (ires == 2)
				res = FPGA_BUSY;
			else
				OPAE_ERR("error opening vfio device: %s", addr);
			goto out_destroy;
		}
	}

	return FPGA_OK;

out_destroy:
	opae_free(pair->device);
	opae_free(pair);
	*ppair = NULL;
	return res;
}

STATIC fpga_result vfio_reset(const vfio_pci_device_t *dev,
			      volatile uint8_t *port_base)
{
	ASSERT_NOT_NULL(dev);
	ASSERT_NOT_NULL(port_base);
	OPAE_ERR("fpgaReset() is not implemented "
		 "for this vfio device.");
	return FPGA_OK;
}

STATIC int vfio_walk(vfio_pci_device_t *dev)
{
	int res = 0;
	volatile uint8_t *mmio = NULL;
	size_t size = 0;
	vfio_pair_t *pair = NULL;
	fpga_guid bar0_guid;
	const uint32_t bar = 0;
	vfio_token *tok;
	struct opae_vfio *v;

	res = open_vfio_pair(dev->addr, &pair);
	if (res) {
		OPAE_DBG("error opening vfio device: %s",
			 dev->addr);
		return res;
	}

	v = pair->device;

	// look for legacy FME guids in BAR 0
	if (opae_vfio_region_get(v, bar, (uint8_t **)&mmio, &size)) {
		OPAE_ERR("error getting BAR 0");
		res = 2;
		goto close;
	}

	// get the GUID at offset 0x8
	res = vfio_get_guid(((uint64_t *)mmio)+1, bar0_guid);
	if (res) {
		OPAE_ERR("error reading guid");
		goto close;
	}

	// walk our known list of FME guids
	// and compare each one to the one read into bar0_guid
	for (const char **u = fme_drivers; *u; u++) {
		fpga_guid uuid;

		res = uuid_parse(*u, uuid);
		if (res) {
			OPAE_ERR("error parsing uuid: %s", *u);
			goto close;
		}
		if (!uuid_compare(uuid, bar0_guid)) {
			// we found a legacy FME in BAR0, walk it
			res = walk_fme(dev, v, mmio, (int)bar);
			goto close;
		}
	}

	// If we got here, we didn't find an FME/Port. In this case,
	// treat all of BAR0 as an FPGA_ACCELERATOR.
	tok = vfio_get_token(dev, bar, FPGA_ACCELERATOR);
	if (!tok) {
		OPAE_ERR("failed to find token during walk");
		res = -1;
		goto close;
	}

	tok->mmio_size = size;
	tok->user_mmio_count = 1;
	tok->user_mmio[bar] = 0;
	tok->ops.reset = vfio_reset;
	vfio_get_guid(1+(uint64_t *)mmio, tok->hdr.guid);

	// only check BAR 0 for an FPGA_ACCELERATOR, skip other BARs

close:
	close_vfio_pair(&pair);
	return res;
}

fpga_result __VFIO_API__ vfio_fpgaOpen(fpga_token token, fpga_handle *handle, int flags)
{
	fpga_result res = FPGA_EXCEPTION;
	vfio_token *_token;
	vfio_handle *_handle;
	pthread_mutexattr_t mattr;
	uint8_t *mmio = NULL;
	size_t size = 0;

	ASSERT_NOT_NULL(token);
	ASSERT_NOT_NULL(handle);

	_token = token_check(token);
	ASSERT_NOT_NULL(_token);

	if (pthread_mutexattr_init(&mattr)) {
		OPAE_ERR("Failed to init handle mutex attr");
		return FPGA_EXCEPTION;
	}

	if (flags & FPGA_OPEN_SHARED) {
		OPAE_DBG("shared mode ignored at this time");
	}

	_handle = opae_calloc(1, sizeof(vfio_handle));
	if (!_handle) {
		OPAE_ERR("Failed to allocate memory for handle");
		res = FPGA_NO_MEMORY;
		goto out_attr_destroy;
	}

	// mark data structure as valid
	if (pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE) ||
	    pthread_mutex_init(&_handle->lock, &mattr)) {
		OPAE_ERR("Failed to init handle mutex");
		res = FPGA_EXCEPTION;
		goto out_attr_destroy;
	}

	_handle->magic = VFIO_HANDLE_MAGIC;
	_handle->token = clone_token(_token);

	res = open_vfio_pair(_token->device->addr, &_handle->vfio_pair);
	if (res) {
		OPAE_DBG("error opening vfio device: %s",
			 _token->device->addr);
		goto out_attr_destroy;
	}

	if (opae_vfio_region_get(_handle->vfio_pair->device,
				 _token->region, &mmio, &size)) {
		OPAE_ERR("error opening vfio region");
		res = FPGA_EXCEPTION;
		goto out_attr_destroy;
	}
	_handle->mmio_base = (volatile uint8_t *)mmio;
	_handle->mmio_size = size;

	_handle->flags = 0;
#if defined(__i386__) || defined(__x86_64__) || defined(__ia64__)
#if GCC_VERSION >= 40900
	__builtin_cpu_init();
	if (__builtin_cpu_supports("avx512f")) {
		_handle->flags |= OPAE_FLAG_HAS_AVX512;
	}
#endif // GCC_VERSION
#endif // x86

	*handle = _handle;
	res = FPGA_OK;
out_attr_destroy:
	pthread_mutexattr_destroy(&mattr);
	if (res && _handle) {
		pthread_mutex_destroy(&_handle->lock);
		if (_handle->vfio_pair)
			close_vfio_pair(&_handle->vfio_pair);
		if (_handle->token) {
			if (_handle->token->parent)
				opae_free(_handle->token->parent);
			opae_free(_handle->token);
		}
		_handle->magic = 0;
		opae_free(_handle);
	}
	return res;
}

fpga_result __VFIO_API__ vfio_fpgaClose(fpga_handle handle)
{
	fpga_result res = FPGA_OK;
	vfio_handle *h;
	vfio_token *t;

	h = handle_check_and_lock(handle);
	ASSERT_NOT_NULL(h);

	t = token_check(h->token);
	if (t) {
		if (t->parent)
			opae_free(t->parent);
		opae_free(t);
	} else {
		OPAE_ERR("invalid token in handle");
	}

	if (h->flags & OPAE_FLAG_SVA_FD_VALID) {
		// Release PASID and shared virtual addressing
		opae_close(h->sva_fd);
		h->flags &= ~(OPAE_FLAG_SVA_FD_VALID | OPAE_FLAG_PASID_VALID);
	}

	close_vfio_pair(&h->vfio_pair);

	if (pthread_mutex_unlock(&h->lock) ||
	    pthread_mutex_destroy(&h->lock)) {
		OPAE_ERR("error unlocking/destroying handle mutex");
		res = FPGA_EXCEPTION;
	}

	h->magic = 0;
	opae_free(h);
	return res;
}

fpga_result __VFIO_API__ vfio_fpgaReset(fpga_handle handle)
{
	int err;
	fpga_result res = FPGA_NOT_SUPPORTED;
	vfio_handle *h;
	vfio_token *t;

	h = handle_check_and_lock(handle);
	ASSERT_NOT_NULL(h);

	t = h->token;
	if ((t->hdr.objtype == FPGA_ACCELERATOR) && t->ops.reset) {
		res = t->ops.reset(t->device, h->mmio_base);
	}

	opae_mutex_unlock(err, &h->lock);

	return res;
}

fpga_result vfio_get_guid(uint64_t *src, fpga_guid guid)
{
	ASSERT_NOT_NULL(src);

	uint64_t *dst = (uint64_t *)guid;
	*dst = bswap_64(*(src+1));
	*(dst+1) = bswap_64(*src);

	return FPGA_OK;
}

fpga_result __VFIO_API__ vfio_fpgaUpdateProperties(fpga_token token, fpga_properties prop)
{
	vfio_token *t;
	struct _fpga_properties *_prop;
	int err;

	t = token_check(token);
	ASSERT_NOT_NULL(t);

	_prop = opae_validate_and_lock_properties(prop);
	if (!_prop) {
		OPAE_ERR("Invalid properties object");
		return FPGA_INVALID_PARAM;
	}

	_prop->valid_fields = 0;

	_prop->vendor_id = t->device->vendor;
	SET_FIELD_VALID(_prop, FPGA_PROPERTY_VENDORID);

	_prop->device_id = t->device->device;
	SET_FIELD_VALID(_prop, FPGA_PROPERTY_DEVICEID);

	_prop->subsystem_vendor_id = t->device->subsystem_vendor;
	SET_FIELD_VALID(_prop, FPGA_PROPERTY_SUB_VENDORID);

	_prop->subsystem_device_id = t->device->subsystem_device;
	SET_FIELD_VALID(_prop, FPGA_PROPERTY_SUB_DEVICEID);

	_prop->segment = t->device->bdf.segment;
	SET_FIELD_VALID(_prop, FPGA_PROPERTY_SEGMENT);

	_prop->bus = t->device->bdf.bus;
	SET_FIELD_VALID(_prop, FPGA_PROPERTY_BUS);

	_prop->device = t->device->bdf.device;
	SET_FIELD_VALID(_prop, FPGA_PROPERTY_DEVICE);

	_prop->function = t->device->bdf.function;
	SET_FIELD_VALID(_prop, FPGA_PROPERTY_FUNCTION);

	_prop->socket_id = t->device->numa_node;
	SET_FIELD_VALID(_prop, FPGA_PROPERTY_SOCKETID);

	_prop->object_id = ((uint64_t)t->device->bdf.bdf) << 32 | t->region;
	SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJECTID);

	_prop->objtype = t->hdr.objtype;
	SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);

	_prop->interface = FPGA_IFC_VFIO;
	SET_FIELD_VALID(_prop, FPGA_PROPERTY_INTERFACE);

	if (t->hdr.objtype == FPGA_ACCELERATOR) {
		fpga_result res;
		vfio_pair_t *pair = NULL;

		_prop->parent = NULL;
		CLEAR_FIELD_VALID(_prop, FPGA_PROPERTY_PARENT);

		memcpy(_prop->guid, t->hdr.guid, sizeof(fpga_guid));
		SET_FIELD_VALID(_prop, FPGA_PROPERTY_GUID);

		_prop->u.accelerator.num_mmio = t->user_mmio_count;
		SET_FIELD_VALID(_prop, FPGA_PROPERTY_NUM_MMIO);

		_prop->u.accelerator.num_interrupts = t->num_afu_irqs;
		SET_FIELD_VALID(_prop, FPGA_PROPERTY_NUM_INTERRUPTS);

		SET_FIELD_VALID(_prop, FPGA_PROPERTY_ACCELERATOR_STATE);
		res = open_vfio_pair(t->device->addr, &pair);
		if (res == FPGA_OK) {
			close_vfio_pair(&pair);
			_prop->u.accelerator.state =
				t->afu_state = FPGA_ACCELERATOR_UNASSIGNED;
		} else {
			_prop->u.accelerator.state =
				t->afu_state = FPGA_ACCELERATOR_ASSIGNED;
		}

	} else {
		memcpy(_prop->guid, t->compat_id, sizeof(fpga_guid));
		SET_FIELD_VALID(_prop, FPGA_PROPERTY_GUID);

		_prop->u.fpga.bbs_id = t->bitstream_id;
		SET_FIELD_VALID(_prop, FPGA_PROPERTY_BBSID);

		_prop->u.fpga.bbs_version.major =
			FPGA_BBS_VER_MAJOR(t->bitstream_id);
		_prop->u.fpga.bbs_version.minor =
			FPGA_BBS_VER_MINOR(t->bitstream_id);
		_prop->u.fpga.bbs_version.patch =
			FPGA_BBS_VER_PATCH(t->bitstream_id);
		SET_FIELD_VALID(_prop, FPGA_PROPERTY_BBSVERSION);

		_prop->u.fpga.num_slots = t->num_ports;
		SET_FIELD_VALID(_prop, FPGA_PROPERTY_NUM_SLOTS);
	}

	opae_mutex_unlock(err, &_prop->lock);
	return FPGA_OK;
}

fpga_result __VFIO_API__ vfio_fpgaGetProperties(fpga_token token, fpga_properties *prop)
{
	struct _fpga_properties *_prop = NULL;
	fpga_result result = FPGA_OK;
	int err;

	ASSERT_NOT_NULL(prop);

	result = fpgaGetProperties(NULL, (fpga_properties *)&_prop);
	if (result)
		return result;

	if (token) {
		result = vfio_fpgaUpdateProperties(token, _prop);
		if (result)
			goto out_free;
	}

	*prop = (fpga_properties)_prop;
	return result;

out_free:
	err = pthread_mutex_destroy(&_prop->lock);
	if (err)
		OPAE_ERR("pthread_mutex_destroy() failed");
	opae_free(_prop);
	return result;
}

fpga_result __VFIO_API__ vfio_fpgaGetPropertiesFromHandle(fpga_handle handle, fpga_properties *prop)
{
	vfio_handle *h;
	vfio_token *t;
	fpga_result res;
	int err;

	ASSERT_NOT_NULL(prop);

	h = handle_check_and_lock(handle);
	ASSERT_NOT_NULL(h);

	t = h->token;
	if (!t) {
		res = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	res = vfio_fpgaGetProperties(t, prop);

out_unlock:
	opae_mutex_unlock(err, &h->lock);

	return res;
}

static inline volatile uint8_t *get_user_offset(vfio_handle *h,
						uint32_t mmio_num,
						uint32_t offset)
{
	uint32_t user_mmio = h->token->user_mmio[mmio_num];

	return h->mmio_base + user_mmio + offset;
}


fpga_result __VFIO_API__ vfio_fpgaWriteMMIO64(fpga_handle handle,
					      uint32_t mmio_num,
					      uint64_t offset,
					      uint64_t value)
{
	vfio_handle *h;
	vfio_token *t;
	fpga_result res = FPGA_OK;
	int err;

	h = handle_check_and_lock(handle);
	ASSERT_NOT_NULL(h);

	t = h->token;

	if (t->hdr.objtype == FPGA_DEVICE) {
		res = FPGA_NOT_SUPPORTED;
		goto out_unlock;
	}

	if (mmio_num >= USER_MMIO_MAX) {
		res = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	*((volatile uint64_t *)get_user_offset(h, mmio_num, offset)) = value;

out_unlock:
	opae_mutex_unlock(err, &h->lock);
	return res;
}

fpga_result __VFIO_API__ vfio_fpgaReadMMIO64(fpga_handle handle,
					     uint32_t mmio_num,
					     uint64_t offset,
					     uint64_t *value)
{
	vfio_handle *h;
	vfio_token *t;
	fpga_result res = FPGA_OK;
	int err;

	h = handle_check_and_lock(handle);
	ASSERT_NOT_NULL(h);

	t = h->token;

	if (t->hdr.objtype == FPGA_DEVICE) {
		res = FPGA_NOT_SUPPORTED;
		goto out_unlock;
	}

	if (mmio_num >= USER_MMIO_MAX) {
		res = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	*value = *((volatile uint64_t *)get_user_offset(h, mmio_num, offset));

out_unlock:
	opae_mutex_unlock(err, &h->lock);
	return res;
}

fpga_result __VFIO_API__ vfio_fpgaWriteMMIO32(fpga_handle handle,
					      uint32_t mmio_num,
					      uint64_t offset,
					      uint32_t value)
{
	vfio_handle *h;
	vfio_token *t;
	fpga_result res = FPGA_OK;
	int err;

	h = handle_check_and_lock(handle);
	ASSERT_NOT_NULL(h);

	t = h->token;

	if (t->hdr.objtype == FPGA_DEVICE) {
		res = FPGA_NOT_SUPPORTED;
		goto out_unlock;
	}

	if (mmio_num >= USER_MMIO_MAX) {
		res = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	*((volatile uint32_t *)get_user_offset(h, mmio_num, offset)) = value;

out_unlock:
	opae_mutex_unlock(err, &h->lock);
	return res;
}

fpga_result __VFIO_API__ vfio_fpgaReadMMIO32(fpga_handle handle,
					     uint32_t mmio_num,
					     uint64_t offset,
					     uint32_t *value)
{
	vfio_handle *h;
	vfio_token *t;
	fpga_result res = FPGA_OK;
	int err;

	h = handle_check_and_lock(handle);
	ASSERT_NOT_NULL(h);

	t = h->token;

	if (t->hdr.objtype == FPGA_DEVICE) {
		res = FPGA_NOT_SUPPORTED;
		goto out_unlock;
	}

	if (mmio_num >= USER_MMIO_MAX) {
		res = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	*value = *((volatile uint32_t *)get_user_offset(h, mmio_num, offset));

out_unlock:
	opae_mutex_unlock(err, &h->lock);

	return res;
}

#if defined(__i386__) || defined(__x86_64__) || defined(__ia64__) && GCC_VERSION >= 40900
static inline void copy512(const void *src, void *dst)
{
    asm volatile("vmovdqu64 (%0), %%zmm0;"
		 "vmovdqu64 %%zmm0, (%1);"
		 :
		 : "r"(src), "r"(dst));
}
#else
static inline void copy512(const void *src, void *dst)
{
	UNUSED_PARAM(src);
	UNUSED_PARAM(dst);
}
#endif // x86

fpga_result __VFIO_API__ vfio_fpgaWriteMMIO512(fpga_handle handle,
					       uint32_t mmio_num,
					       uint64_t offset,
					       const void *value)
{
	vfio_handle *h;
	vfio_token *t;
	fpga_result res = FPGA_OK;
	int err;

	if ((offset % 64) != 0) {
		OPAE_ERR("Misaligned MMIO access");
		return FPGA_INVALID_PARAM;
	}

	h = handle_check_and_lock(handle);
	ASSERT_NOT_NULL(h);

	t = h->token;

	if ((t->hdr.objtype == FPGA_DEVICE) ||
	    !(h->flags & OPAE_FLAG_HAS_AVX512)) {
		res = FPGA_NOT_SUPPORTED;
		goto out_unlock;
	}

	if (mmio_num >= USER_MMIO_MAX) {
		res = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	copy512(value, (uint8_t *)get_user_offset(h, mmio_num, offset));

out_unlock:
	opae_mutex_unlock(err, &h->lock);
	return res;
}

fpga_result __VFIO_API__ vfio_fpgaMapMMIO(fpga_handle handle,
					  uint32_t mmio_num,
					  uint64_t **mmio_ptr)
{
	vfio_handle *h;
	fpga_result res = FPGA_OK;
	int err;

	h = handle_check_and_lock(handle);
	ASSERT_NOT_NULL(h);

	if (mmio_num >= USER_MMIO_MAX) {
		res = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	/* Store return value only if return pointer has allocated memory */
	if (mmio_ptr)
		*mmio_ptr = (uint64_t *)get_user_offset(h, mmio_num, 0);

out_unlock:
	opae_mutex_unlock(err, &h->lock);
	return res;
}

fpga_result __VFIO_API__ vfio_fpgaUnmapMMIO(fpga_handle handle,
					    uint32_t mmio_num)
{
	vfio_handle *h;
	fpga_result res = FPGA_OK;
	int err;

	h = handle_check_and_lock(handle);
	ASSERT_NOT_NULL(h);

	if (mmio_num >= USER_MMIO_MAX) {
		res = FPGA_INVALID_PARAM;
	}

	opae_mutex_unlock(err, &h->lock);
	return res;
}

STATIC vfio_token *find_token(const vfio_pci_device_t *dev,
			      uint32_t region,
			      fpga_objtype objtype)
{
	vfio_token *t = dev->tokens;

	while (t) {
		if ((t->region == region) && (t->hdr.objtype == objtype))
			return t;
		t = t->next;
	}

	return NULL;
}

vfio_token *vfio_get_token(vfio_pci_device_t *dev,
			   uint32_t region,
			   fpga_objtype objtype)
{
	vfio_token *t;

	t = find_token(dev, region, objtype);
	if (t)
		return t;

	t = (vfio_token *)opae_calloc(1, sizeof(vfio_token));
	if (!t) {
		OPAE_ERR("Failed to allocate memory for vfio_token");
		return NULL;
	}

	t->hdr.magic = VFIO_TOKEN_MAGIC;
	t->device = dev;
	t->region = region;
	t->hdr.objtype = objtype;

	t->next = dev->tokens;
	dev->tokens = t;

	return t;
}

STATIC bool pci_matches_filter(const fpga_properties filter, vfio_pci_device_t *dev)
{
	struct _fpga_properties *_prop = (struct _fpga_properties *)filter;

	if (FIELD_VALID(_prop, FPGA_PROPERTY_SEGMENT))
		if (_prop->segment != dev->bdf.segment)
			return false;
	if (FIELD_VALID(_prop, FPGA_PROPERTY_BUS))
		if (_prop->bus != dev->bdf.bus)
			return false;
	if (FIELD_VALID(_prop, FPGA_PROPERTY_DEVICE))
		if (_prop->device != dev->bdf.device)
			return false;
	if (FIELD_VALID(_prop, FPGA_PROPERTY_FUNCTION))
		if (_prop->function != dev->bdf.function)
			return false;
	if (FIELD_VALID(_prop, FPGA_PROPERTY_SOCKETID))
		if (_prop->socket_id != dev->numa_node)
			return false;
	if (FIELD_VALID(_prop, FPGA_PROPERTY_VENDORID))
		if (_prop->vendor_id != dev->vendor)
			return false;
	if (FIELD_VALID(_prop, FPGA_PROPERTY_DEVICEID))
		if (_prop->device_id != dev->device)
			return false;
	if (FIELD_VALID(_prop, FPGA_PROPERTY_SUB_VENDORID))
		if (_prop->subsystem_vendor_id != dev->subsystem_vendor)
			return false;
	if (FIELD_VALID(_prop, FPGA_PROPERTY_SUB_DEVICEID))
		if (_prop->subsystem_device_id != dev->subsystem_device)
			return false;

	return true;
}

STATIC bool pci_matches_filters(const fpga_properties *filters,
				uint32_t num_filters,
				vfio_pci_device_t *dev)
{
	if (!filters)
		return true;

	for (uint32_t i = 0; i < num_filters; ++i) {
		if (pci_matches_filter(filters[i], dev))
			return true;
	}

	return false;
}

STATIC bool matches_filter(const fpga_properties filter, vfio_token *t)
{
	struct _fpga_properties *_prop = (struct _fpga_properties *)filter;

	if (FIELD_VALID(_prop, FPGA_PROPERTY_PARENT)) {
		fpga_token_header *parent_hdr =
			(fpga_token_header *)_prop->parent;

		if (!parent_hdr)
			return false;

		if (!fpga_is_parent_child(parent_hdr, &t->hdr))
			return false;
	}

	if (FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE)) {
		if (_prop->objtype != t->hdr.objtype)
			return false;

		if ((t->hdr.objtype == FPGA_ACCELERATOR) &&
		    FIELD_VALID(_prop, FPGA_PROPERTY_ACCELERATOR_STATE))
			if (_prop->u.accelerator.state != t->afu_state)
				return false;

		if ((t->hdr.objtype == FPGA_ACCELERATOR) &&
		    FIELD_VALID(_prop, FPGA_PROPERTY_NUM_INTERRUPTS))
			if (_prop->u.accelerator.num_interrupts != t->num_afu_irqs)
				return false;
	}

	if (FIELD_VALID(_prop, FPGA_PROPERTY_OBJECTID))
		if (_prop->object_id != t->hdr.object_id)
			return false;

	if (FIELD_VALID(_prop, FPGA_PROPERTY_GUID)) {
		if ((t->hdr.objtype == FPGA_ACCELERATOR) &&
		    memcmp(_prop->guid, t->hdr.guid, sizeof(fpga_guid)))
			return false;
		if ((t->hdr.objtype == FPGA_DEVICE) &&
		    memcmp(_prop->guid, t->compat_id, sizeof(fpga_guid)))
			return false;
	}
	if (FIELD_VALID(_prop, FPGA_PROPERTY_INTERFACE))
		if (_prop->interface != FPGA_IFC_VFIO)
			return false;

	return true;
}

STATIC bool matches_filters(const fpga_properties *filters,
			    uint32_t num_filters,
			    vfio_token *t)
{
	if (!filters)
		return true;

	for (uint32_t i = 0; i < num_filters; ++i) {
		if (matches_filter(filters[i], t))
			return true;
	}

	return false;
}

STATIC uint32_t vfio_irq_count(struct opae_vfio *device)
{
	struct opae_vfio_device_irq *irq =
		device->device.irqs;

	while (irq) {
		if (irq->index == VFIO_PCI_MSIX_IRQ_INDEX)
			return irq->count;
		irq = irq->next;
	}

	return 0;
}

fpga_result __VFIO_API__ vfio_fpgaEnumerate(const fpga_properties *filters,
			       uint32_t num_filters, fpga_token *tokens,
			       uint32_t max_tokens, uint32_t *num_matches)
{
	vfio_pci_device_t *dev = _pci_devices;
	uint32_t matches = 0;

	while (dev) {
		if (pci_matches_filters(filters, num_filters, dev)) {
			vfio_token *tptr;

			vfio_walk(dev);
			tptr = dev->tokens;

			while (tptr) {
				vfio_pair_t *pair = NULL;
				fpga_result res;

				tptr->hdr.vendor_id = (uint16_t)tptr->device->vendor;
				tptr->hdr.device_id = (uint16_t)tptr->device->device;
				tptr->hdr.subsystem_vendor_id = tptr->device->subsystem_vendor;
				tptr->hdr.subsystem_device_id = tptr->device->subsystem_device;
				tptr->hdr.segment = tptr->device->bdf.segment;
				tptr->hdr.bus = tptr->device->bdf.bus;
				tptr->hdr.device = tptr->device->bdf.device;
				tptr->hdr.function = tptr->device->bdf.function;
				tptr->hdr.interface = FPGA_IFC_VFIO;
				//tptr->hdr.objtype = <already populated>
				tptr->hdr.object_id = ((uint64_t)tptr->device->bdf.bdf) << 32 | tptr->region;
				if (tptr->hdr.objtype == FPGA_DEVICE)
					memcpy(tptr->hdr.guid, tptr->compat_id, sizeof(fpga_guid));

				res = open_vfio_pair(tptr->device->addr, &pair);
				if (res == FPGA_OK) {
					tptr->num_afu_irqs = vfio_irq_count(pair->device);

					close_vfio_pair(&pair);
					tptr->afu_state = FPGA_ACCELERATOR_UNASSIGNED;
				} else {
					tptr->afu_state = FPGA_ACCELERATOR_ASSIGNED;
				}

				if (matches_filters(filters, num_filters, tptr)) {
					if (matches < max_tokens) {
						tokens[matches] =
							clone_token(tptr);
					}
					++matches;
				}
				tptr = tptr->next;
			}
		}
		dev = dev->next;
	}

	*num_matches = matches;

	return FPGA_OK;
}

fpga_result __VFIO_API__ vfio_fpgaCloneToken(fpga_token src, fpga_token *dst)
{
	vfio_token *_src;
	vfio_token *_dst;

	if (!src || !dst) {
		OPAE_ERR("src or dst token is NULL");
		return FPGA_INVALID_PARAM;
	}

	_src = (vfio_token *)src;
	if (_src->hdr.magic != VFIO_TOKEN_MAGIC) {
		OPAE_ERR("Invalid src token");
		return FPGA_INVALID_PARAM;
	}

	_dst = opae_malloc(sizeof(vfio_token));
	if (!_dst) {
		OPAE_ERR("Failed to allocate memory for vfio_token");
		return FPGA_NO_MEMORY;
	}

	memcpy(_dst, _src, sizeof(vfio_token));
	_dst->next = NULL;

	*dst = _dst;
	return FPGA_OK;
}

fpga_result __VFIO_API__ vfio_fpgaDestroyToken(fpga_token *token)
{
	vfio_token *t;

	if (!token || !*token) {
		OPAE_ERR("invalid token pointer");
		return FPGA_INVALID_PARAM;
	}

	t = (vfio_token *)*token;
	if (t->hdr.magic == VFIO_TOKEN_MAGIC) {
		t->hdr.magic = 0;
		opae_free(t);
		return FPGA_OK;
	}

	return FPGA_INVALID_PARAM;
}

#define HUGE_1G (1*1024*1024*1024)
#define HUGE_2M (2*1024*1024)
#define ROUND_UP(N, M) ((N + M - 1) & ~(M-1))

fpga_result __VFIO_API__ vfio_fpgaPrepareBuffer(fpga_handle handle,
						uint64_t len,
						void **buf_addr,
						uint64_t *wsid,
						int flags)
{
	vfio_handle *h;
	uint8_t *virt = NULL;
	struct opae_vfio_buffer *binfo = NULL;

	if (flags & FPGA_BUF_PREALLOCATED) {
		if (!buf_addr && !len) {
			return FPGA_OK;
			/* Special case: respond FPGA_OK when
			** !buf_addr and !len as an indication that
			** FPGA_BUF_PREALLOCATED is supported by the
			** library.
			*/
		} else if (!buf_addr) {
			OPAE_ERR("got FPGA_BUF_PREALLOCATED but NULL buf");
			return FPGA_INVALID_PARAM;
		} else {
			virt = *buf_addr;
		}
	}

	ASSERT_NOT_NULL(wsid);

	h = handle_check(handle);
	ASSERT_NOT_NULL(h);

	fpga_result res = FPGA_EXCEPTION;

	struct opae_vfio *v = h->vfio_pair->device;
	uint64_t iova = 0;
	size_t sz;
	if (len > HUGE_2M)
		sz = ROUND_UP(len, HUGE_1G);
	else if (len > 4096)
		sz = ROUND_UP(len, HUGE_2M);
	else
		sz = 4096;
	if (opae_vfio_buffer_allocate_ex(v, &sz, &virt, &iova, flags)) {
		OPAE_DBG("could not allocate buffer");
		return FPGA_EXCEPTION;
	}
	binfo = opae_vfio_buffer_info(v, virt);

	if (!binfo) {
		OPAE_ERR("error allocating buffer metadata");
		if (opae_vfio_buffer_free(v, virt)) {
			OPAE_ERR("error freeing vfio buffer");
		}
		res = FPGA_NO_MEMORY;
		goto out_free;
	}

	*buf_addr = virt;
	*wsid = (uint64_t)binfo;

	res = FPGA_OK;
out_free:
	if (res) {
		if (virt && opae_vfio_buffer_free(v, virt)) {
			OPAE_ERR("error freeing vfio buffer");
		}
	}
	return res;
}

fpga_result __VFIO_API__ vfio_fpgaReleaseBuffer(fpga_handle handle,
						uint64_t wsid)
{
	vfio_handle *h = handle_check(handle);

	ASSERT_NOT_NULL(h);

	struct opae_vfio *v = h->vfio_pair->device;
	struct opae_vfio_buffer *binfo = (struct opae_vfio_buffer *)wsid;
	fpga_result res = FPGA_OK;

	ASSERT_NOT_NULL(binfo);

	if (opae_vfio_buffer_free(v, binfo->buffer_ptr)) {
		OPAE_ERR("error freeing vfio buffer");
		res = FPGA_NOT_FOUND;
	}

	return res;
}

fpga_result __VFIO_API__ vfio_fpgaGetIOAddress(fpga_handle handle,
					       uint64_t wsid,
					       uint64_t *ioaddr)
{
	UNUSED_PARAM(handle);

	ASSERT_NOT_NULL(ioaddr);

	struct opae_vfio_buffer *binfo = (struct opae_vfio_buffer *)wsid;

	ASSERT_NOT_NULL(binfo);

	*ioaddr = binfo->buffer_iova;

	return FPGA_OK;
}

fpga_result __VFIO_API__ vfio_fpgaBindSVA(fpga_handle handle, uint32_t *pasid)
{
	vfio_handle *h;
	char path[PATH_MAX];
	int fd;
	int err;
	fpga_result res = FPGA_OK;

	h = handle_check_and_lock(handle);
	ASSERT_NOT_NULL(h);

	struct opae_vfio *v = h->vfio_pair->device;
	if (!v || !v->cont_pciaddr) {
		res = FPGA_NO_DRIVER;
		goto out_unlock;
	}

	if (h->flags & OPAE_FLAG_PASID_VALID) {
		// vfio_fpgaBindSVA() was already called. Return the same result.
		if (pasid)
			*pasid = h->pasid;
		goto out_unlock;
	}

	// Currently, the dfl-pci driver provides support for allocating a PASID
	// and binding it to the FPGA in the IOMMU. At some point, VFIO and the
	// IOMMUFD will also support shared virtual addressing. When ready, this
	// function can be updated to call libopaevfio.

	// dfl-pci creates a device at /dev/dfl-pci-sva/<pci_addr> for all
	// ports that support shared virtual memory. If the device isn't found
	// then assume PASID is not supported, either by the host or the FPGA.
	snprintf(path, sizeof(path), "/dev/dfl-pci-sva/%s", v->cont_pciaddr);
	fd = opae_open(path, O_RDONLY);
	if (fd >= 0) {
		// Request a shared virtual addressing. On success, the PASID is
		// returned.
		int bind_pasid = opae_ioctl(fd, DFL_PCI_SVA_BIND_DEV);
		if (bind_pasid >= 0) {
			// Success. Hold the file open. Closing the file would release
			// the PASID and disable address sharing.
			if (pasid)
				*pasid = bind_pasid;
			h->pasid = bind_pasid;
			h->sva_fd = fd;
			h->flags |= (OPAE_FLAG_SVA_FD_VALID | OPAE_FLAG_PASID_VALID);
			goto out_unlock;
		}
		opae_close(fd);
	}

	res = FPGA_NOT_SUPPORTED;

out_unlock:
	opae_mutex_unlock(err, &h->lock);

	return res;
}

fpga_result __VFIO_API__ vfio_fpgaCreateEventHandle(fpga_event_handle *event_handle)
{
	vfio_event_handle *_veh;
	fpga_result res = FPGA_OK;
	pthread_mutexattr_t mattr;
	int err;

	ASSERT_NOT_NULL(event_handle);

	_veh = opae_malloc(sizeof(vfio_event_handle));
	if (!_veh) {
		OPAE_ERR("Out of memory");
		return FPGA_NO_MEMORY;
	}

	_veh->magic = VFIO_EVENT_HANDLE_MAGIC;
	_veh->flags = 0;

	_veh->fd = eventfd(0, 0);
	if (_veh->fd < 0) {
		OPAE_ERR("eventfd : %s", strerror(errno));
		res = FPGA_EXCEPTION;
		goto out_free;
	}

	if (pthread_mutexattr_init(&mattr)) {
		OPAE_ERR("Failed to init event handle mutex attr");
		res = FPGA_EXCEPTION;
		goto out_free;
	}

	if (pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE) ||
	    pthread_mutex_init(&_veh->lock, &mattr)) {
		OPAE_ERR("Failed to initialize event handle lock");
		res = FPGA_EXCEPTION;
		goto out_attr_destroy;
	}

	pthread_mutexattr_destroy(&mattr);

	*event_handle = (fpga_event_handle)_veh;
	return FPGA_OK;

out_attr_destroy:
	err = pthread_mutexattr_destroy(&mattr);
	if (err) {
		OPAE_ERR("pthread_mutexattr_destroy() failed: %s",
			 strerror(err));
	}
out_free:
	opae_free(_veh);
	return res;
}

fpga_result __VFIO_API__ vfio_fpgaDestroyEventHandle(fpga_event_handle *event_handle)
{
	vfio_event_handle *_veh;
	int err;

	ASSERT_NOT_NULL(event_handle);

	_veh = event_handle_check_and_lock(*event_handle);
	ASSERT_NOT_NULL(_veh);

	if (close(_veh->fd) < 0) {
		OPAE_ERR("eventfd : %s", strerror(errno));
		err = pthread_mutex_unlock(&_veh->lock);
		if (err)
			OPAE_ERR("pthread_mutex_unlock() failed: %s",
				 strerror(err));
		return (errno == EBADF) ? FPGA_INVALID_PARAM : FPGA_EXCEPTION;
	}

	_veh->magic = 0;

	opae_mutex_unlock(err, &_veh->lock);
	err = pthread_mutex_destroy(&_veh->lock);
	if (err)
		OPAE_ERR("pthread_mutex_destroy() failed: %s",
			 strerror(errno));

	opae_free(_veh);

	*event_handle = NULL;
	return FPGA_OK;
}

fpga_result __VFIO_API__ vfio_fpgaGetOSObjectFromEventHandle(const fpga_event_handle eh,
							     int *fd)
{
	vfio_event_handle *_veh;
	int err;

	ASSERT_NOT_NULL(eh);
	ASSERT_NOT_NULL(fd);

	_veh = event_handle_check_and_lock(eh);
	ASSERT_NOT_NULL(_veh);

	*fd = _veh->fd;

	opae_mutex_unlock(err, &_veh->lock);

	return FPGA_OK;
}

STATIC fpga_result register_event(vfio_handle *_h,
				  fpga_event_type event_type,
				  vfio_event_handle *_veh,
				  uint32_t flags)
{
	switch (event_type) {
	case FPGA_EVENT_ERROR:
		OPAE_ERR("Error interrupts are not currently supported.");
		return FPGA_NOT_SUPPORTED;

	case FPGA_EVENT_INTERRUPT:
		_veh->flags = flags;

		if (opae_vfio_irq_enable(_h->vfio_pair->device,
					 VFIO_PCI_MSIX_IRQ_INDEX,
					 flags,
					 _veh->fd)) {
			OPAE_ERR("Couldn't enable MSIX IRQ %u : %s",
				 flags, strerror(errno));
			return FPGA_EXCEPTION;
		}

		return FPGA_OK;
	case FPGA_EVENT_POWER_THERMAL:
		OPAE_ERR("Thermal interrupts are not currently supported.");
		return FPGA_NOT_SUPPORTED;
	default:
		OPAE_ERR("Invalid event type");
		return FPGA_EXCEPTION;
	}
}

fpga_result __VFIO_API__ vfio_fpgaRegisterEvent(fpga_handle handle,
						fpga_event_type event_type,
						fpga_event_handle event_handle,
						uint32_t flags)
{
	vfio_handle *_h;
	vfio_event_handle *_veh;
	fpga_result res = FPGA_EXCEPTION;
	int err;

	ASSERT_NOT_NULL(handle);
	ASSERT_NOT_NULL(event_handle);

	_h = handle_check_and_lock(handle);
	ASSERT_NOT_NULL(_h);

	_veh = event_handle_check_and_lock(event_handle);
	if (!_veh)
		goto out_unlock_handle;

	res = register_event(_h, event_type, _veh, flags);

	opae_mutex_unlock(err, &_veh->lock);

out_unlock_handle:
	opae_mutex_unlock(err, &_h->lock);
	return res;
}

STATIC fpga_result unregister_event(vfio_handle *_h,
				    fpga_event_type event_type,
				    vfio_event_handle *_veh)
{
	switch (event_type) {
	case FPGA_EVENT_ERROR:
		OPAE_ERR("Error interrupts are not currently supported.");
		return FPGA_NOT_SUPPORTED;
	case FPGA_EVENT_INTERRUPT:
		if (opae_vfio_irq_disable(_h->vfio_pair->device,
					  VFIO_PCI_MSIX_IRQ_INDEX,
					  _veh->flags)) {
			OPAE_ERR("Couldn't disable MSIX IRQ %u : %s",
				 _veh->flags, strerror(errno));
			return FPGA_EXCEPTION;
		}
		return FPGA_OK;
	case FPGA_EVENT_POWER_THERMAL:
		OPAE_ERR("Thermal interrupts are not currently supported.");
		return FPGA_NOT_SUPPORTED;
	default:
		OPAE_ERR("Invalid event type");
		return FPGA_EXCEPTION;
	}
}

fpga_result __VFIO_API__ vfio_fpgaUnregisterEvent(fpga_handle handle,
						  fpga_event_type event_type,
						  fpga_event_handle event_handle)
{
	vfio_handle *_h;
	vfio_event_handle *_veh;
	fpga_result res = FPGA_EXCEPTION;
	int err;

	ASSERT_NOT_NULL(handle);
	ASSERT_NOT_NULL(event_handle);

	_h = handle_check_and_lock(handle);
	ASSERT_NOT_NULL(_h);

	_veh = event_handle_check_and_lock(event_handle);
	if (!_veh)
		goto out_unlock_handle;

	res = unregister_event(_h, event_type, _veh);

	opae_mutex_unlock(err, &_veh->lock);

out_unlock_handle:
	opae_mutex_unlock(err, &_h->lock);
	return res;
}

// Copyright(c) 2023, Intel Corporation
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

#include "opae_uio.h"
#include "dfl.h"

#include "opae_int.h"
#include "props.h"
#include "cfg-file.h"
#include "mock/opae_std.h"

#define UIO_TOKEN_MAGIC 0xFF1010FF
#define UIO_HANDLE_MAGIC ~UIO_TOKEN_MAGIC
#define UIO_EVENT_HANDLE_MAGIC 0x5a7447a5

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

static uio_pci_device_t *_pci_devices;

STATIC int read_pci_attr(const char *addr, const char *attr, char *value, size_t max)
{
	int res = FPGA_OK;
	char path[PATH_MAX];

	snprintf(path, sizeof(path), "/sys/bus/pci/devices/%s/%s", addr, attr);
	FILE *fp = fopen(path, "r");

	if (!fp) {
		ERR("error opening: %s", path);
		return FPGA_EXCEPTION;
	}
	if (!fread(value, 1, max, fp)) {
		ERR("error reading from: %s", path);
		res = FPGA_EXCEPTION;
	}
	fclose(fp);
	return res;
}

STATIC int read_pci_attr_u32(const char *addr, const char *attr, uint32_t *value)
{
	char str_value[64];
	char *endptr = NULL;
	int res = read_pci_attr(addr, attr, str_value, sizeof(str_value));

	if (res)
		return res;
	uint32_t v = strtoul(str_value, &endptr, 0);

	if (endptr == str_value) {
		ERR("error parsing string: %s", str_value);
		return FPGA_EXCEPTION;
	}
	*value = v;
	return FPGA_OK;
}

STATIC int parse_pcie_info(uio_pci_device_t *device, const char *addr)
{
	char err[128] = {0};
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

void free_token_list(uio_token *tokens)
{
	while (tokens) {
		uio_token *trash = tokens;
		tokens = tokens->next;
		opae_free(trash);
	}
}

void uio_free_device_list(void)
{
	while (_pci_devices) {
		uio_pci_device_t *trash = _pci_devices;
		_pci_devices = _pci_devices->next;
		free_token_list(trash->tokens);
		opae_free(trash);
	}
}

STATIC uio_pci_device_t *find_pci_device(const char addr[PCIADDR_MAX],
					 const char dfl_dev[DFL_DEV_MAX])
{
	uio_pci_device_t *p = _pci_devices;

	while (p) {
		if (!strncmp(p->addr, addr, PCIADDR_MAX) &&
		    !strncmp(p->dfl_dev, dfl_dev, DFL_DEV_MAX))
			return p;
		p = p->next;
	}

	return NULL;
}

uio_pci_device_t *uio_get_pci_device(const char addr[PCIADDR_MAX],
				     const char dfl_dev[DFL_DEV_MAX])
{
	uint32_t value;
	uio_pci_device_t *p = find_pci_device(addr, dfl_dev);

	if (p)
		return p;

	p = (uio_pci_device_t *)opae_calloc(1, sizeof(uio_pci_device_t));
	if (!p) {
		ERR("Failed to allocate memory for uio_pci_device_t");
		return NULL;
	}

	strncpy(p->addr, addr, PCIADDR_MAX-1);
	strncpy(p->dfl_dev, dfl_dev, DFL_DEV_MAX-1);

	if (read_pci_attr_u32(addr, "vendor", &p->vendor)) {
		OPAE_ERR("error reading 'vendor' attribute: %s", addr);
		goto free;
	}

	value = 0;
	if (read_pci_attr_u32(addr, "subsystem_vendor", &value)) {
		OPAE_ERR("error reading 'subsystem_vendor' attribute: %s",
			 addr);
		goto free;
	}
	p->subsystem_vendor = (uint16_t)value;

	if (read_pci_attr_u32(addr, "device", &p->device)) {
		OPAE_ERR("error reading 'device' attribute: %s", addr);
		goto free;
	}

	value = 0;
	if (read_pci_attr_u32(addr, "subsystem_device", &value)) {
		OPAE_ERR("error reading 'subsystem_device' attribute: %s",
			 addr);
		goto free;
	}
	p->subsystem_device = (uint16_t)value;

	if (read_pci_attr_u32(addr, "numa_node", &p->numa_node)) {
		OPAE_ERR("error opening 'numa_node' attribute: %s", addr);
		goto free;
	}

	if (parse_pcie_info(p, addr)) {
		OPAE_ERR("error parsing pcie address: %s", addr);
		goto free;
	}

	p->next = _pci_devices;
	_pci_devices = p;
	return p;

free:
	opae_free(p);
	return NULL;
}

libopae_config_data *opae_u_supported_devices;

STATIC bool pci_device_matches(const libopae_config_data *c,
			       uint16_t vid,
			       uint16_t did,
			       uint16_t svid,
			       uint16_t sdid)
{
	if (strcmp(c->module_library, "libopae-u.so"))
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

bool pci_device_supported(const char *pcie_addr)
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

	for (i = 0 ; opae_u_supported_devices[i].module_library ; ++i) {
		if (pci_device_matches(&opae_u_supported_devices[i],
				       (uint16_t)vendor, (uint16_t)device,
				       (uint16_t)subsystem_vendor,
				       (uint16_t)subsystem_device))
			return true;
	}

	return false;
}

int uio_pci_discover(void)
{
	int res = 1;
	const char *gpattern = "/sys/bus/dfl/drivers/uio_dfl/dfl_dev.*";
	glob_t pg;
	int gres;
	char rlbuf[PATH_MAX];

	gres = opae_glob(gpattern, 0, NULL, &pg);

	if (gres) {
		OPAE_DBG("uio-dfl not bound to any PCIe endpoint");
		res = 0;
		goto free;
	}
	if (!pg.gl_pathc) {
		goto free;
	}

	for (size_t i = 0; i < pg.gl_pathc; ++i) {
		const char *dfl_device;
		char *pcie_addr;
		uio_pci_device_t *dev;

		dfl_device = strrchr(pg.gl_pathv[i], '/');
		if (!dfl_device) {
			OPAE_ERR("gl_pathv misformatted");
			continue;
		}
		++dfl_device;

		memset(rlbuf, 0, sizeof(rlbuf));
		if (opae_readlink(pg.gl_pathv[i], rlbuf, PATH_MAX - 1) == -1) {
			OPAE_ERR("readlink failed");
			continue;
		}

		pcie_addr = strstr(rlbuf, "/fpga_region/region");
		if (!pcie_addr) {
			OPAE_ERR("link misformatted");
			continue;
		}
		*pcie_addr = '\0'; // null-terminate the address.
		pcie_addr -= 12; // back up to the beginning of the PCIe address.

		if (!pci_device_supported(pcie_addr))
			continue;

		dev = uio_get_pci_device(pcie_addr, dfl_device);

		if (!dev) {
			OPAE_ERR("error with pci address: %s", pcie_addr);
		}
	}
	res = 0;

free:
	opae_globfree(&pg);
	return res;
}

uio_token *clone_token(uio_token *src)
{
	ASSERT_NOT_NULL_RESULT(src, NULL);
	if (src->hdr.magic != UIO_TOKEN_MAGIC)
		return NULL;
	uio_token *token = (uio_token *)opae_malloc(sizeof(uio_token));

	if (!token) {
		ERR("Failed to allocate memory for uio_token");
		return NULL;
	}
	memcpy(token, src, sizeof(uio_token));
	if (src->parent)
		token->parent = clone_token(src->parent);

	token->next = NULL;

	return token;
}

uio_token *token_check(fpga_token token)
{
	uio_token *t;

	ASSERT_NOT_NULL_RESULT(token, NULL);

	t = (uio_token *)token;
	if (t->hdr.magic != UIO_TOKEN_MAGIC) {
		OPAE_ERR("invalid token magic");
		return NULL;
	}

	return t;
}

uio_handle *handle_check(fpga_handle handle)
{
	uio_handle *h;

	ASSERT_NOT_NULL_RESULT(handle, NULL);

	h = (uio_handle *)handle;
	if (h->magic != UIO_HANDLE_MAGIC) {
		OPAE_ERR("invalid handle magic");
		return NULL;
	}

	return h;
}

uio_event_handle *event_handle_check(fpga_event_handle event_handle)
{
	uio_event_handle *eh;

	ASSERT_NOT_NULL_RESULT(event_handle, NULL);

	eh = (uio_event_handle *)event_handle;
	if (eh->magic != UIO_EVENT_HANDLE_MAGIC) {
		OPAE_ERR("invalid event handle magic");
		return NULL;
	}

	return eh;
}

uio_handle *handle_check_and_lock(fpga_handle handle)
{
	int res;
	uio_handle *h;

	h = handle_check(handle);
	if (h)
		return opae_mutex_lock(res, &h->lock) ? NULL : h;

	return NULL;
}

uio_event_handle *
event_handle_check_and_lock(fpga_event_handle event_handle)
{
	int res;
	uio_event_handle *eh;
		
	eh = event_handle_check(event_handle);
	if (eh)
		return opae_mutex_lock(res, &eh->lock) ? NULL : eh;

	return NULL;
}

static fpga_result uio_reset(const uio_pci_device_t *dev,
			     volatile uint8_t *port_base)
{
	ASSERT_NOT_NULL(dev);
	ASSERT_NOT_NULL(port_base);
	OPAE_ERR("fpgaReset for uio is not implemented yet");
	return FPGA_OK;
}

int uio_walk(uio_pci_device_t *dev)
{
	int res = 0;
	volatile uint8_t *mmio = NULL;
	size_t size = 0;
	struct opae_uio uio;
	fpga_guid bar0_guid;
	uio_token *tok;

	res = opae_uio_open(&uio, dev->dfl_dev);
	if (res) {
		OPAE_DBG("error opening uio device: %s %s",
			 dev->addr, dev->dfl_dev);
		return res;
	}

	// look for legacy FME guids in BAR 0
	if (opae_uio_region_get(&uio, 0, (uint8_t **)&mmio, &size)) {
		OPAE_ERR("error getting BAR 0");
		res = 2;
		goto close;
	}

	// get the GUID at offset 0x8
	res = uio_get_guid(((uint64_t *)mmio)+1, bar0_guid);
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
			res = walk_fme(dev, &uio, mmio, 0);
			goto close;
		}
	}

	// If we got here, we didn't find an FME/Port. In this case,
	// treat all of BAR0 as an FPGA_ACCELERATOR.
	tok = uio_get_token(dev, 0, FPGA_ACCELERATOR);
	if (!tok) {
		OPAE_ERR("failed to find token during walk");
		res = -1;
		goto close;
	}

	tok->mmio_size = size;
	tok->user_mmio_count = 1;
	tok->user_mmio[0] = 0;
	tok->ops.reset = uio_reset;
	uio_get_guid(1+(uint64_t *)mmio, tok->hdr.guid);

	// only check BAR 0 for an FPGA_ACCELERATOR, skip other BARs

close:
	opae_uio_close(&uio);
	return res;
}

fpga_result __UIO_API__ uio_fpgaOpen(fpga_token token, fpga_handle *handle, int flags)
{
	fpga_result res = FPGA_EXCEPTION;
	uio_token *_token;
	uio_handle *_handle;
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

	_handle = opae_calloc(1, sizeof(uio_handle));
	if (!_handle) {
		ERR("Failed to allocate memory for handle");
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

	_handle->magic = UIO_HANDLE_MAGIC;
	_handle->token = clone_token(_token);

	res = opae_uio_open(&_handle->uio, _token->device->dfl_dev);
	if (res) {
		OPAE_DBG("error opening uio device: %s %s",
			 _token->device->addr, _token->device->dfl_dev);
		goto out_attr_destroy;
	}

	if (opae_uio_region_get(&_handle->uio,
				 _token->region, &mmio, &size)) {
		OPAE_ERR("error opening uio region");
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
		opae_uio_close(&_handle->uio);
		_handle->magic = 0;
		opae_free(_handle);
	}
	return res;
}

fpga_result __UIO_API__ uio_fpgaClose(fpga_handle handle)
{
	fpga_result res = FPGA_OK;
	uio_handle *h;

	h = handle_check_and_lock(handle);
	ASSERT_NOT_NULL(h);

	if (token_check(h->token))
		opae_free(h->token);
	else
		OPAE_ERR("invalid token in handle");

	opae_uio_close(&h->uio);

	if (pthread_mutex_unlock(&h->lock) ||
	    pthread_mutex_destroy(&h->lock)) {
		OPAE_ERR("error unlocking/destroying handle mutex");
		res = FPGA_EXCEPTION;
	}

	h->magic = 0;
	opae_free(h);
	return res;
}

fpga_result __UIO_API__ uio_fpgaReset(fpga_handle handle)
{
	int err;
	fpga_result res = FPGA_NOT_SUPPORTED;
	uio_handle *h;
	uio_token *t;

	h = handle_check_and_lock(handle);
	ASSERT_NOT_NULL(h);

	t = h->token;
	if ((t->hdr.objtype == FPGA_ACCELERATOR) && t->ops.reset) {
		res = t->ops.reset(t->device, h->mmio_base);
	}

	opae_mutex_unlock(err, &h->lock);

	return res;
}

fpga_result uio_get_guid(uint64_t *h, fpga_guid guid)
{
	ASSERT_NOT_NULL(h);

	uint64_t *ptr = (uint64_t *)guid;
	*ptr = bswap_64(*(h+1));
	*(ptr+1) = bswap_64(*h);
	return FPGA_OK;
}

fpga_result __UIO_API__ uio_fpgaUpdateProperties(fpga_token token, fpga_properties prop)
{
	uio_token *t = token_check(token);

	ASSERT_NOT_NULL(t);

	struct _fpga_properties *_prop = (struct _fpga_properties *)prop;

	if (!_prop) {
		return FPGA_EXCEPTION;
	}
	if (_prop->magic != FPGA_PROPERTY_MAGIC) {
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

	_prop->interface = FPGA_IFC_UIO;
	SET_FIELD_VALID(_prop, FPGA_PROPERTY_INTERFACE);

	if (t->hdr.objtype == FPGA_ACCELERATOR) {
		fpga_result res;
		struct opae_uio uio;

		_prop->parent = NULL;
		CLEAR_FIELD_VALID(_prop, FPGA_PROPERTY_PARENT);

		memcpy(_prop->guid, t->hdr.guid, sizeof(fpga_guid));
		SET_FIELD_VALID(_prop, FPGA_PROPERTY_GUID);

		_prop->u.accelerator.num_mmio = t->user_mmio_count;
		SET_FIELD_VALID(_prop, FPGA_PROPERTY_NUM_MMIO);

		_prop->u.accelerator.num_interrupts = t->num_afu_irqs;
		SET_FIELD_VALID(_prop, FPGA_PROPERTY_NUM_INTERRUPTS);

		SET_FIELD_VALID(_prop, FPGA_PROPERTY_ACCELERATOR_STATE);
		res = opae_uio_open(&uio, t->device->dfl_dev);
		if (res == 0) {
			opae_uio_close(&uio);
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

	return FPGA_OK;
}

fpga_result __UIO_API__ uio_fpgaGetProperties(fpga_token token, fpga_properties *prop)
{
	ASSERT_NOT_NULL(prop);
	struct _fpga_properties *_prop = NULL;
	fpga_result result = FPGA_OK;


	result = fpgaGetProperties(NULL, (fpga_properties *)&_prop);
	if (result)
		return result;
	if (token) {
		result = uio_fpgaUpdateProperties(token, _prop);
		if (result)
			goto out_free;
	}
	*prop = (fpga_properties)_prop;

	return result;
out_free:
	free(_prop);
	return result;
}

fpga_result __UIO_API__ uio_fpgaGetPropertiesFromHandle(fpga_handle handle, fpga_properties *prop)
{
	uio_handle *h;
	uio_token *t;

	ASSERT_NOT_NULL(handle);
	ASSERT_NOT_NULL(prop);

	h = handle_check(handle);
	ASSERT_NOT_NULL(h);

	t = h->token;
	ASSERT_NOT_NULL(t);

	return uio_fpgaGetProperties(t, prop);
}

static inline volatile uint8_t *get_user_offset(uio_handle *h,
						uint32_t mmio_num,
						uint32_t offset)
{
	uint32_t user_mmio = h->token->user_mmio[mmio_num];

	return h->mmio_base + user_mmio + offset;
}


fpga_result __UIO_API__ uio_fpgaWriteMMIO64(fpga_handle handle,
				 uint32_t mmio_num,
				 uint64_t offset,
				 uint64_t value)
{
	uio_handle *h;
	uio_token *t;
	fpga_result res = FPGA_OK;
	int err;

	h = handle_check_and_lock(handle);
	ASSERT_NOT_NULL(h);

	t = h->token;

	if (t->hdr.objtype == FPGA_DEVICE) {
		res = FPGA_NOT_SUPPORTED;
		goto out_unlock;
	}

	if (mmio_num > t->user_mmio_count) {
		res = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	*((volatile uint64_t *)get_user_offset(h, mmio_num, offset)) = value;

out_unlock:
	opae_mutex_unlock(err, &h->lock);
	return res;
}

fpga_result __UIO_API__ uio_fpgaReadMMIO64(fpga_handle handle,
				uint32_t mmio_num,
				uint64_t offset,
				uint64_t *value)
{
	uio_handle *h;
	uio_token *t;
	fpga_result res = FPGA_OK;
	int err;

	h = handle_check_and_lock(handle);
	ASSERT_NOT_NULL(h);

	t = h->token;

	if (t->hdr.objtype == FPGA_DEVICE) {
		res = FPGA_NOT_SUPPORTED;
		goto out_unlock;
	}

	if (mmio_num > t->user_mmio_count) {
		res = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	*value = *((volatile uint64_t *)get_user_offset(h, mmio_num, offset));

out_unlock:
	opae_mutex_unlock(err, &h->lock);
	return res;
}

fpga_result __UIO_API__ uio_fpgaWriteMMIO32(fpga_handle handle,
				 uint32_t mmio_num,
				 uint64_t offset,
				 uint32_t value)
{
	uio_handle *h;
	uio_token *t;
	fpga_result res = FPGA_OK;
	int err;

	h = handle_check_and_lock(handle);
	ASSERT_NOT_NULL(h);

	t = h->token;

	if (t->hdr.objtype == FPGA_DEVICE) {
		res = FPGA_NOT_SUPPORTED;
		goto out_unlock;
	}

	if (mmio_num > t->user_mmio_count) {
		res = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	*((volatile uint32_t *)get_user_offset(h, mmio_num, offset)) = value;

out_unlock:
	opae_mutex_unlock(err, &h->lock);
	return res;
}

fpga_result __UIO_API__ uio_fpgaReadMMIO32(fpga_handle handle,
				uint32_t mmio_num,
				uint64_t offset,
				uint32_t *value)
{
	uio_handle *h;
	uio_token *t;
	fpga_result res = FPGA_OK;
	int err;

	h = handle_check_and_lock(handle);
	ASSERT_NOT_NULL(h);

	t = h->token;

	if (t->hdr.objtype == FPGA_DEVICE) {
		res = FPGA_NOT_SUPPORTED;
		goto out_unlock;
	}

	if (mmio_num > t->user_mmio_count) {
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

fpga_result __UIO_API__ uio_fpgaWriteMMIO512(fpga_handle handle,
				 uint32_t mmio_num,
				 uint64_t offset,
				 const void *value)
{
	uio_handle *h;
	uio_token *t;
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

	if (mmio_num > t->user_mmio_count) {
		res = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	copy512(value, (uint8_t *)get_user_offset(h, mmio_num, offset));

out_unlock:
	opae_mutex_unlock(err, &h->lock);
	return res;
}

fpga_result __UIO_API__ uio_fpgaMapMMIO(fpga_handle handle,
			     uint32_t mmio_num,
			     uint64_t **mmio_ptr)
{
	uio_handle *h;
	uio_token *t;
	fpga_result res = FPGA_OK;
	int err;

	h = handle_check_and_lock(handle);
	ASSERT_NOT_NULL(h);

	t = h->token;

	if (mmio_num > t->user_mmio_count) {
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

fpga_result __UIO_API__ uio_fpgaUnmapMMIO(fpga_handle handle,
			       uint32_t mmio_num)
{
	uio_handle *h;
	uio_token *t;
	fpga_result res = FPGA_OK;
	int err;

	h = handle_check_and_lock(handle);
	ASSERT_NOT_NULL(h);

	t = h->token;

	if (mmio_num > t->user_mmio_count) {
		res = FPGA_INVALID_PARAM;
	}

	opae_mutex_unlock(err, &h->lock);
	return res;
}

STATIC uio_token *find_token(const uio_pci_device_t *dev,
			     uint32_t region,
			     fpga_objtype objtype)
{
	uio_token *t = dev->tokens;

	while (t) {
		if ((t->region == region) && (t->hdr.objtype == objtype))
			return t;
		t = t->next;
	}

	return NULL;
}

uio_token *uio_get_token(uio_pci_device_t *dev,
			 uint32_t region,
			 fpga_objtype objtype)
{
	uio_token *t;

	t = find_token(dev, region, objtype);
	if (t)
		return t;

	t = (uio_token *)opae_calloc(1, sizeof(uio_token));
	if (!t) {
		ERR("Failed to allocate memory for uio_token");
		return NULL;
	}

	t->hdr.magic = UIO_TOKEN_MAGIC;
	t->device = dev;
	t->region = region;
	t->hdr.objtype = objtype;

	t->next = dev->tokens;
	dev->tokens = t;

	return t;
}

bool pci_matches_filter(const fpga_properties *filter, uio_pci_device_t *dev)
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

bool pci_matches_filters(const fpga_properties *filters, uint32_t num_filters,
			 uio_pci_device_t *dev)
{
	if (!filters)
		return true;

	for (uint32_t i = 0; i < num_filters; ++i) {
		if (pci_matches_filter(filters[i], dev))
			return true;
	}

	return false;
}

bool matches_filter(const fpga_properties *filter, uio_token *t)
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
		if (_prop->interface != FPGA_IFC_UIO)
			return false;

	return true;
}

bool matches_filters(const fpga_properties *filters, uint32_t num_filters,
		     uio_token *t)
{
	if (!filters)
		return true;

	for (uint32_t i = 0; i < num_filters; ++i) {
		if (matches_filter(filters[i], t))
			return true;
	}

	return false;
}

fpga_result __UIO_API__ uio_fpgaEnumerate(const fpga_properties *filters,
			       uint32_t num_filters, fpga_token *tokens,
			       uint32_t max_tokens, uint32_t *num_matches)
{
	uio_pci_device_t *dev = _pci_devices;
	uint32_t matches = 0;

	while (dev) {
		if (pci_matches_filters(filters, num_filters, dev)) {
			uio_token *tptr;

			uio_walk(dev);
			tptr = dev->tokens;

			while (tptr) {
				struct opae_uio uio;
				fpga_result res;

				tptr->hdr.vendor_id = (uint16_t)tptr->device->vendor;
				tptr->hdr.device_id = (uint16_t)tptr->device->device;
				tptr->hdr.subsystem_vendor_id = tptr->device->subsystem_vendor;
				tptr->hdr.subsystem_device_id = tptr->device->subsystem_device;
				tptr->hdr.segment = tptr->device->bdf.segment;
				tptr->hdr.bus = tptr->device->bdf.bus;
				tptr->hdr.device = tptr->device->bdf.device;
				tptr->hdr.function = tptr->device->bdf.function;
				tptr->hdr.interface = FPGA_IFC_UIO;
				//tptr->hdr.objtype = <already populated>
				tptr->hdr.object_id = ((uint64_t)tptr->device->bdf.bdf) << 32 | tptr->region;
				if (tptr->hdr.objtype == FPGA_DEVICE)
					memcpy(tptr->hdr.guid, tptr->compat_id, sizeof(fpga_guid));

				res = opae_uio_open(&uio, tptr->device->dfl_dev);
				if (res == 0) {
					tptr->num_afu_irqs = 1;
					opae_uio_close(&uio);
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

fpga_result __UIO_API__ uio_fpgaCloneToken(fpga_token src, fpga_token *dst)
{
	uio_token *_src;
	uio_token *_dst;

	if (!src || !dst) {
		OPAE_ERR("src or dst token is NULL");
		return FPGA_INVALID_PARAM;
	}

	_src = (uio_token *)src;
	if (_src->hdr.magic != UIO_TOKEN_MAGIC) {
		OPAE_ERR("Invalid src token");
		return FPGA_INVALID_PARAM;
	}

	_dst = opae_malloc(sizeof(uio_token));
	if (!_dst) {
		OPAE_ERR("Failed to allocate memory for uio_token");
		return FPGA_NO_MEMORY;
	}

	memcpy(_dst, _src, sizeof(uio_token));
	_dst->next = NULL;

	*dst = _dst;
	return FPGA_OK;
}

fpga_result __UIO_API__ uio_fpgaDestroyToken(fpga_token *token)
{
	uio_token *t;

	if (!token || !*token) {
		OPAE_ERR("invalid token pointer");
		return FPGA_INVALID_PARAM;
	}

	t = (uio_token *)*token;
	if (t->hdr.magic == UIO_TOKEN_MAGIC) {
		t->hdr.magic = 0;
		opae_free(t);
		return FPGA_OK;
	}

	return FPGA_INVALID_PARAM;
}

fpga_result __UIO_API__ uio_fpgaCreateEventHandle(fpga_event_handle *event_handle)
{
	uio_event_handle *_ueh;
	fpga_result res = FPGA_OK;
	pthread_mutexattr_t mattr;
	int err;

	ASSERT_NOT_NULL(event_handle);

	_ueh = opae_malloc(sizeof(uio_event_handle));
	if (!_ueh) {
		OPAE_ERR("Out of memory");
		return FPGA_NO_MEMORY;
	}

	_ueh->magic = UIO_EVENT_HANDLE_MAGIC;
	_ueh->fd = -1;
	_ueh->flags = 0;

	if (pthread_mutexattr_init(&mattr)) {
		OPAE_ERR("Failed to init event handle mutex attr");
		res = FPGA_EXCEPTION;
		goto out_free;
	}

	if (pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE) ||
	    pthread_mutex_init(&_ueh->lock, &mattr)) {
		OPAE_ERR("Failed to initialize event handle lock");
		res = FPGA_EXCEPTION;
		goto out_attr_destroy;
	}

	pthread_mutexattr_destroy(&mattr);

	*event_handle = (fpga_event_handle)_ueh;
	return FPGA_OK;

out_attr_destroy:
	err = pthread_mutexattr_destroy(&mattr);
	if (err) {
		OPAE_ERR("pthread_mutexattr_destroy() failed: %s",
			 strerror(err));
	}
out_free:
	opae_free(_ueh);
	return res;
}

fpga_result __UIO_API__ uio_fpgaDestroyEventHandle(fpga_event_handle *event_handle)
{
	uio_event_handle *_ueh;
	int err;

	ASSERT_NOT_NULL(event_handle);

	_ueh = event_handle_check_and_lock(*event_handle);
	ASSERT_NOT_NULL(_ueh);

	_ueh->magic = 0;

	opae_mutex_unlock(err, &_ueh->lock);
	err = pthread_mutex_destroy(&_ueh->lock);
	if (err)
		OPAE_ERR("pthread_mutex_destroy() failed: %s",
			 strerror(errno));

	opae_free(_ueh);

	*event_handle = NULL;
	return FPGA_OK;
}

fpga_result __UIO_API__ uio_fpgaGetOSObjectFromEventHandle(const fpga_event_handle eh,
						int *fd)
{
	uio_event_handle *_ueh;
	int err;

	ASSERT_NOT_NULL(eh);
	ASSERT_NOT_NULL(fd);

	_ueh = event_handle_check_and_lock(eh);
	ASSERT_NOT_NULL(_ueh);

	*fd = _ueh->fd;

	opae_mutex_unlock(err, &_ueh->lock);

	return FPGA_OK;
}

STATIC fpga_result register_event(uio_handle *_h,
				  fpga_event_type event_type,
				  uio_event_handle *_ueh,
				  uint32_t flags)
{
	UNUSED_PARAM(_h);
	switch (event_type) {
	case FPGA_EVENT_ERROR:
		OPAE_ERR("Error interrupts are not currently supported.");
		return FPGA_NOT_SUPPORTED;

	case FPGA_EVENT_INTERRUPT:
		_ueh->flags = flags;
		return FPGA_OK;

	case FPGA_EVENT_POWER_THERMAL:
		OPAE_ERR("Thermal interrupts are not currently supported.");
		return FPGA_NOT_SUPPORTED;
	default:
		OPAE_ERR("Invalid event type");
		return FPGA_EXCEPTION;
	}
}

fpga_result __UIO_API__ uio_fpgaRegisterEvent(fpga_handle handle,
				   fpga_event_type event_type,
				   fpga_event_handle event_handle,
				   uint32_t flags)
{
	uio_handle *_h;
	uio_event_handle *_ueh;
	fpga_result res = FPGA_EXCEPTION;
	int err;

	ASSERT_NOT_NULL(handle);
	ASSERT_NOT_NULL(event_handle);

	_h = handle_check_and_lock(handle);
	ASSERT_NOT_NULL(_h);

	_ueh = event_handle_check_and_lock(event_handle);
	if (!_ueh)
		goto out_unlock_handle;

	_ueh->fd = _h->uio.device_fd;
	res = register_event(_h, event_type, _ueh, flags);

	opae_mutex_unlock(err, &_ueh->lock);

out_unlock_handle:
	opae_mutex_unlock(err, &_h->lock);
	return res;
}

STATIC fpga_result unregister_event(uio_handle *_h,
				    fpga_event_type event_type,
				    uio_event_handle *_ueh)
{
	UNUSED_PARAM(_h);
	UNUSED_PARAM(_ueh);
	switch (event_type) {
	case FPGA_EVENT_ERROR:
		OPAE_ERR("Error interrupts are not currently supported.");
		return FPGA_NOT_SUPPORTED;
	case FPGA_EVENT_INTERRUPT:
		return FPGA_OK;
	case FPGA_EVENT_POWER_THERMAL:
		OPAE_ERR("Thermal interrupts are not currently supported.");
		return FPGA_NOT_SUPPORTED;
	default:
		OPAE_ERR("Invalid event type");
		return FPGA_EXCEPTION;
	}
}

fpga_result __UIO_API__ uio_fpgaUnregisterEvent(fpga_handle handle,
				     fpga_event_type event_type,
				     fpga_event_handle event_handle)
{
	uio_handle *_h;
	uio_event_handle *_ueh;
	fpga_result res = FPGA_EXCEPTION;
	int err;

	ASSERT_NOT_NULL(handle);
	ASSERT_NOT_NULL(event_handle);

	_h = handle_check_and_lock(handle);
	ASSERT_NOT_NULL(_h);

	_ueh = event_handle_check_and_lock(event_handle);
	if (!_ueh)
		goto out_unlock_handle;

	res = unregister_event(_h, event_type, _ueh);

	opae_mutex_unlock(err, &_ueh->lock);

out_unlock_handle:
	opae_mutex_unlock(err, &_h->lock);
	return res;
}

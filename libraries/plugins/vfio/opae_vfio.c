// Copyright(c) 2020-2022, Intel Corporation
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

#define _GNU_SOURCE
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

#include "props.h"
#include "opae_vfio.h"
#include "dfl.h"

#define BAR_MAX 6
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
		char *ptr = _p + _m.rm_so;                                     \
		char *endptr = NULL;                                           \
		_v = strtoul(ptr, &endptr, _b);                                \
		if (endptr == ptr) {                                           \
			OPAE_MSG("error parsing int");                         \
			goto _l;                                               \
		}                                                              \
	} while (0)


const char *fme_drivers[] = {
	"bfaf2ae9-4a52-46e3-82fe-38f0f9e17764",
	0
};

typedef struct _vfio_buffer {
	uint8_t *virtual;
	uint64_t iova;
	uint64_t wsid;
	size_t size;
	struct opae_vfio *vfio_device;
	struct _vfio_buffer *next;
} vfio_buffer;


static pci_device_t *_pci_devices;
static vfio_buffer *_vfio_buffers;
static pthread_mutex_t _buffers_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

STATIC int read_pci_link(const char *addr, const char *link, char *value, size_t max)
{
	char path[PATH_MAX];
	char fullpath[PATH_MAX];

	snprintf(path, sizeof(path), "/sys/bus/pci/devices/%s/%s", addr, link);
	if (!realpath(path, fullpath)) {
		if (errno == ENOENT)
			return 1;
		ERR("error reading path: %s", path);
		return 2;
	}
	char *p = strrchr(fullpath, '/');

	if (!p) {
		ERR("error finding '/' in path: %s", fullpath);
		return 2;
	}
	strncpy(value, p+1, max);
	return 0;
}

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

STATIC int parse_pcie_info(pci_device_t *device, char *addr)
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

void free_token_list(vfio_token *tokens)
{
	while (tokens) {
		vfio_token *t = tokens;

		tokens = tokens->next;
		free(t);
	}
}

void free_device_list(void)
{
	while (_pci_devices) {
		pci_device_t *p = _pci_devices;

		_pci_devices = _pci_devices->next;
		free_token_list(p->tokens);
		free(p);
	}
}

void free_buffer_list(void)
{
	vfio_buffer *ptr = _vfio_buffers;
	vfio_buffer *tmp = NULL;

	while (ptr) {
		tmp = ptr;
		ptr = tmp->next;
		free(tmp);
	}
}


pci_device_t *find_pci_device(char addr[PCIADDR_MAX])
{
	pci_device_t *p = _pci_devices;

	while (p && strncmp(p->addr, addr, PCIADDR_MAX))
		p = p->next;
	return p;
}

pci_device_t *get_pci_device(char addr[PCIADDR_MAX])
{
	uint32_t value;
	pci_device_t *p = find_pci_device(addr);

	if (p)
		return p;
	p = (pci_device_t *)malloc(sizeof(pci_device_t));
	if (!p) {
		ERR("Failed to allocate memory for pci_device_t");
		return NULL;
	}
	memset(p, 0, sizeof(pci_device_t));

	strncpy(p->addr, addr, PCIADDR_MAX-1);

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
	free(p);
	return NULL;
}

STATIC struct {
	uint16_t vendor;
	uint16_t device;
} supported_devices[] = {
	{ 0x1c2c, 0x1000 },
	{ 0x1c2c, 0x1001 },
	{ 0x8086, 0xbcbd },
	{ 0x8086, 0xbcc0 },
	{ 0x8086, 0xbcc1 },
	{ 0x8086, 0x09c4 },
	{ 0x8086, 0x09c5 },
	{ 0x8086, 0x0b2b },
	{ 0x8086, 0x0b2c },
	{ 0x8086, 0x0b30 },
	{ 0x8086, 0x0b31 },
	{ 0x8086, 0xaf00 },
	{ 0x8086, 0xaf01 },
	{ 0x8086, 0xbcce },
	{ 0x8086, 0xbccf },
	{      0,      0 },
};

bool pci_device_supported(const char *pcie_addr)
{
	uint32_t vendor = 0;
	uint32_t device = 0;
	size_t i;

	if (read_pci_attr_u32(pcie_addr, "vendor", &vendor) ||
	    read_pci_attr_u32(pcie_addr, "device", &device)) {
		OPAE_ERR("couldn't determine VID/DID for %s", pcie_addr);
		return false;
	}

	for (i = 0 ; supported_devices[i].vendor ; ++i) {
		if (((uint16_t)vendor == supported_devices[i].vendor) &&
		    ((uint16_t)device == supported_devices[i].device))
			return true;
	}

	return false;
}

int pci_discover(void)
{
	int res = 1;
	const char *gpattern = "/sys/bus/pci/drivers/vfio-pci/????:??:??.?";
	glob_t pg;
	int gres = glob(gpattern, 0, NULL, &pg);

	if (gres) {
		OPAE_MSG("vfio-pci not bound to any PCIe enpoint");
		return 0;
	}
	if (!pg.gl_pathc) {
		goto free;
	}
	for (size_t i = 0; i < pg.gl_pathc; ++i) {
		char *p = strrchr(pg.gl_pathv[i], '/');

		if (!p) {
			ERR("error with gl_pathv");
			continue;
		}

		if (!pci_device_supported(p + 1))
			continue;

		pci_device_t *dev = get_pci_device(p + 1);

		if (!dev) {
			OPAE_ERR("error with pci address: %s", p + 1);
			continue;
		}
	}
	res = 0;
free:
	globfree(&pg);
	return res;
}

vfio_token *clone_token(vfio_token *src)
{
	ASSERT_NOT_NULL_RESULT(src, NULL);
	if (src->hdr.magic != VFIO_TOKEN_MAGIC)
		return NULL;
	vfio_token *token = (vfio_token *)malloc(sizeof(vfio_token));

	if (!token) {
		ERR("Failed to allocate memory for vfio_token");
		return NULL;
	}
	memcpy(token, src, sizeof(vfio_token));
	if (src->parent)
		token->parent = clone_token(src->parent);
	return token;
}

vfio_token *token_check(fpga_token token)
{
	ASSERT_NOT_NULL_RESULT(token, NULL);
	vfio_token *t = (vfio_token *)token;

	if (t->hdr.magic != VFIO_TOKEN_MAGIC) {
		OPAE_ERR("invalid token magic");
		return NULL;
	}
	return t;
}

vfio_handle *handle_check(fpga_handle handle)
{
	ASSERT_NOT_NULL_RESULT(handle, NULL);
	vfio_handle *h = (vfio_handle *)handle;

	if (h->magic != VFIO_HANDLE_MAGIC) {
		OPAE_ERR("invalid handle magic");
		return NULL;
	}
	return h;
}

vfio_event_handle *event_handle_check(fpga_event_handle event_handle)
{
	ASSERT_NOT_NULL_RESULT(event_handle, NULL);
	vfio_event_handle *eh = (vfio_event_handle *)event_handle;

	if (eh->magic != VFIO_EVENT_HANDLE_MAGIC) {
		OPAE_ERR("invalid event handle magic");
		return NULL;
	}
	return eh;
}

vfio_handle *handle_check_and_lock(fpga_handle handle)
{
	vfio_handle *h = handle_check(handle);

	if (h && pthread_mutex_lock(&h->lock)) {
		OPAE_MSG("failed to lock handle mutex");
		return NULL;
	}
	return h;
}

vfio_event_handle *
event_handle_check_and_lock(fpga_event_handle event_handle)
{
	vfio_event_handle *eh = event_handle_check(event_handle);

	if (eh && pthread_mutex_lock(&eh->lock)) {
		OPAE_ERR("failed to lock event handle mutex");
		return NULL;
	}
	return eh;
}

static int close_vfio_pair(vfio_pair_t **pair)
{
	ASSERT_NOT_NULL(pair);
	ASSERT_NOT_NULL(*pair);
	vfio_pair_t *ptr = *pair;

	if (ptr->device) {
		opae_vfio_close(ptr->device);
		free(ptr->device);
	}
	if (ptr->physfn) {
		opae_vfio_close(ptr->physfn);
		free(ptr->physfn);
	}
	free(ptr);
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

	*ppair = malloc(sizeof(vfio_pair_t));

	if (!*ppair) {
		OPAE_ERR("Failed to allocate memory for vfio_pair_t");
		return FPGA_NO_MEMORY;
	}

	pair = *ppair;
	memset(pair, 0, sizeof(vfio_pair_t));

	pair->device = malloc(sizeof(struct opae_vfio));
	if (!pair->device) {
		OPAE_ERR("Failed to allocate memory for opae_vfio struct");
		free(pair);
		*ppair = NULL;
		return FPGA_NO_MEMORY;
	}
	memset(pair->device, 0, sizeof(struct opae_vfio));

	if (!read_pci_link(addr, "physfn", phys_device, PCIADDR_MAX) &&
	    !read_pci_link(phys_device, "driver", phys_driver,
			   sizeof(phys_driver)) &&
	    strstr(phys_driver, "vfio-pci")) {
		uuid_generate(pair->secret);
		uuid_unparse(pair->secret, secret);
		pair->physfn = malloc(sizeof(struct opae_vfio));
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
				ERR("error opening physfn: %s", phys_device);
			free(pair->physfn);
			pair->physfn = NULL;
			goto out_destroy;
		}
		ires = opae_vfio_secure_open(pair->device, addr, secret);
		if (ires) {
			if (ires == 2)
				res = FPGA_BUSY;
			else
				ERR("error opening vfio device: %s", addr);
			goto out_destroy;
		}
	} else {
		ires = opae_vfio_open(pair->device, addr);
		if (ires) {
			if (ires == 2)
				res = FPGA_BUSY;
			else
				ERR("error opening vfio device: %s", addr);
			goto out_destroy;
		}
	}

	return FPGA_OK;

out_destroy:
	free(pair->device);
	free(pair);
	*ppair = NULL;
	return res;
}

static fpga_result vfio_reset(const pci_device_t *p, volatile uint8_t *port_base)
{
	ASSERT_NOT_NULL(p);
	ASSERT_NOT_NULL(port_base);
	OPAE_ERR("fpgaReset for vfio is not implemented yet");
	return FPGA_OK;
}

int vfio_walk(pci_device_t *p)
{
	int res = 0;

	volatile uint8_t *mmio;
	size_t size;
	vfio_pair_t *pair = NULL;

	res = open_vfio_pair(p->addr, &pair);
	if (res) {
		OPAE_DBG("error opening vfio device: %s", p->addr);
		return res;
	}
	struct opae_vfio *v = pair->device;
	// TODO: check PCIe capabilities (VSEC?) for hints to DFLs



	// look for legacy FME guids in BAR 0
	if (opae_vfio_region_get(v, 0, (uint8_t **)&mmio, &size)) {
		OPAE_ERR("error getting BAR 0");
		res = 2;
		goto close;
	}

	// get the GUID at offset 0x8
	fpga_guid b0_guid;

	res = get_guid(((uint64_t *)mmio)+1, b0_guid);
	if (res) {
		OPAE_ERR("error reading guid");
		goto close;
	}

	// walk our known list of FME guids
	// and compare each one to the one read into b0_guid
	for (const char **u = fme_drivers; *u; u++) {
		fpga_guid uuid;

		res = uuid_parse(*u, uuid);
		if (res) {
			OPAE_ERR("error parsing uuid: %s", *u);
			goto close;
		}
		if (!uuid_compare(uuid, b0_guid)) {
			// we found a legacy FME in BAR0, walk it
			res = walk_fme(p, v, mmio, 0);
			goto close;
		}
	}

	// treat all of BAR0 as an FPGA_ACCELERATOR
	vfio_token *t = get_token(p, 0, FPGA_ACCELERATOR);

	t->mmio_size = size;
	t->user_mmio_count = 1;
	t->user_mmio[0] = 0;
	t->ops.reset = vfio_reset;
	get_guid(1+(uint64_t *)mmio, t->hdr.guid);

	// only check BAR 0 for an FPGA_ACCELERATOR, skip other BARs

close:
	close_vfio_pair(&pair);
	return res;
}

fpga_result vfio_fpgaOpen(fpga_token token, fpga_handle *handle, int flags)
{
	fpga_result res = FPGA_EXCEPTION;
	vfio_token *_token;
	vfio_handle *_handle;
	pthread_mutexattr_t mattr;

	ASSERT_NOT_NULL(token);
	ASSERT_NOT_NULL(handle);
	_token = token_check(token);
	ASSERT_NOT_NULL(_token);

	if (pthread_mutexattr_init(&mattr)) {
		OPAE_MSG("Failed to init handle mutex attr");
		return FPGA_EXCEPTION;
	}

	if (flags & FPGA_OPEN_SHARED) {
		OPAE_MSG("shared mode ignored at this time");
		//return FPGA_INVALID_PARAM;
	}


	_handle = malloc(sizeof(vfio_handle));
	if (_handle == NULL) {
		ERR("Failed to allocate memory for handle");
		res = FPGA_NO_MEMORY;
		goto out_attr_destroy;
	}

	memset(_handle, 0, sizeof(*_handle));

	// mark data structure as valid
	if (pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE) ||
	    pthread_mutex_init(&_handle->lock, &mattr)) {
		OPAE_MSG("Failed to init handle mutex");
		res = FPGA_EXCEPTION;
		goto out_attr_destroy;
	}

	_handle->magic = VFIO_HANDLE_MAGIC;
	_handle->token = clone_token(_token);
	res = open_vfio_pair(_token->device->addr, &_handle->vfio_pair);
	if (res) {
		OPAE_DBG("error opening vfio device");
		goto out_attr_destroy;
	}
	uint8_t *mmio = NULL;
	size_t size;

	if (opae_vfio_region_get(_handle->vfio_pair->device,
				 _token->region, &mmio, &size)) {
		OPAE_ERR("error opening vfio region");
		res = FPGA_EXCEPTION;
		goto out_attr_destroy;
	}
	_handle->mmio_base = (volatile uint8_t *)(mmio);
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
		if (_handle->vfio_pair) {
			close_vfio_pair(&_handle->vfio_pair);
		}
		free(_handle);
	}
	return res;
}

fpga_result vfio_fpgaClose(fpga_handle handle)
{
	fpga_result res = FPGA_OK;
	vfio_handle *h = handle_check_and_lock(handle);

	ASSERT_NOT_NULL(h);

	if (token_check(h->token))
		free(h->token);
	else
		OPAE_MSG("invalid token in handle");

	close_vfio_pair(&h->vfio_pair);
	if (pthread_mutex_unlock(&h->lock) ||
	    pthread_mutex_destroy(&h->lock)) {
		OPAE_MSG("error unlocking/destroying handle mutex");
		res = FPGA_EXCEPTION;
	}
	free(h);
	return res;
}

fpga_result vfio_fpgaReset(fpga_handle handle)
{
	vfio_handle *h = handle_check_and_lock(handle);

	ASSERT_NOT_NULL(h);

	fpga_result res = FPGA_NOT_SUPPORTED;

	vfio_token *t = h->token;

	if (t->hdr.objtype == FPGA_ACCELERATOR && t->ops.reset) {
		res = t->ops.reset(t->device, h->mmio_base);
	}
	pthread_mutex_unlock(&h->lock);
	return res;
}

fpga_result get_guid(uint64_t *h, fpga_guid guid)
{
	ASSERT_NOT_NULL(h);

	uint64_t *ptr = (uint64_t *)guid;
	*ptr = bswap_64(*(h+1));
	*(ptr+1) = bswap_64(*h);
	return FPGA_OK;
}

fpga_result vfio_fpgaUpdateProperties(fpga_token token, fpga_properties prop)
{
	vfio_token *t = token_check(token);

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

	_prop->interface = FPGA_IFC_VFIO;
	SET_FIELD_VALID(_prop, FPGA_PROPERTY_INTERFACE);

	if (t->hdr.objtype == FPGA_ACCELERATOR) {
		vfio_pair_t *pair = NULL;
		fpga_result res;

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

	return FPGA_OK;
}

fpga_result vfio_fpgaGetProperties(fpga_token token, fpga_properties *prop)
{
	ASSERT_NOT_NULL(prop);
	struct _fpga_properties *_prop = NULL;
	fpga_result result = FPGA_OK;


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
	free(_prop);
	return result;
}

fpga_result vfio_fpgaGetPropertiesFromHandle(fpga_handle handle, fpga_properties *prop)
{
	ASSERT_NOT_NULL(prop);
	vfio_handle *h = handle_check(handle);

	ASSERT_NOT_NULL(h);

	vfio_token *t = h->token;

	return vfio_fpgaGetProperties(t, prop);
}

static inline volatile uint8_t *get_user_offset(vfio_handle *h,
						uint32_t mmio_num,
						uint32_t offset)
{
	uint32_t user_mmio = h->token->user_mmio[mmio_num];

	return h->mmio_base + user_mmio + offset;
}


fpga_result vfio_fpgaWriteMMIO64(fpga_handle handle,
				 uint32_t mmio_num,
				 uint64_t offset,
				 uint64_t value)
{
	vfio_handle *h = handle_check(handle);

	ASSERT_NOT_NULL(h);

	vfio_token *t = h->token;

	if (t->hdr.objtype == FPGA_DEVICE)
		return FPGA_NOT_SUPPORTED;
	if (mmio_num > t->user_mmio_count)
		return FPGA_INVALID_PARAM;
	if (pthread_mutex_lock(&h->lock)) {
		OPAE_MSG("error locking handle mutex");
		return FPGA_EXCEPTION;
	}

	*((volatile uint64_t *)get_user_offset(h, mmio_num, offset)) = value;
	pthread_mutex_unlock(&h->lock);
	return FPGA_OK;
}

fpga_result vfio_fpgaReadMMIO64(fpga_handle handle,
				uint32_t mmio_num,
				uint64_t offset,
				uint64_t *value)
{
	vfio_handle *h = handle_check(handle);

	ASSERT_NOT_NULL(h);

	vfio_token *t = h->token;

	if (t->hdr.objtype == FPGA_DEVICE)
		return FPGA_NOT_SUPPORTED;
	if (mmio_num > t->user_mmio_count)
		return FPGA_INVALID_PARAM;
	if (pthread_mutex_lock(&h->lock)) {
		OPAE_MSG("error locking handle mutex");
		return FPGA_EXCEPTION;
	}

	*value = *((volatile uint64_t *)get_user_offset(h, mmio_num, offset));
	pthread_mutex_unlock(&h->lock);
	return FPGA_OK;
}

fpga_result vfio_fpgaWriteMMIO32(fpga_handle handle,
				 uint32_t mmio_num,
				 uint64_t offset,
				 uint32_t value)
{
	vfio_handle *h = handle_check(handle);

	ASSERT_NOT_NULL(h);

	vfio_token *t = h->token;

	if (t->hdr.objtype == FPGA_DEVICE)
		return FPGA_NOT_SUPPORTED;
	if (mmio_num > t->user_mmio_count)
		return FPGA_INVALID_PARAM;
	if (pthread_mutex_lock(&h->lock)) {
		OPAE_MSG("error locking handle mutex");
		return FPGA_EXCEPTION;
	}

	*((volatile uint32_t *)get_user_offset(h, mmio_num, offset)) = value;
	pthread_mutex_unlock(&h->lock);
	return FPGA_OK;
}

fpga_result vfio_fpgaReadMMIO32(fpga_handle handle,
				uint32_t mmio_num,
				uint64_t offset,
				uint32_t *value)
{
	vfio_handle *h = handle_check(handle);

	ASSERT_NOT_NULL(h);

	vfio_token *t = h->token;

	if (t->hdr.objtype == FPGA_DEVICE)
		return FPGA_NOT_SUPPORTED;
	if (mmio_num > t->user_mmio_count)
		return FPGA_INVALID_PARAM;
	if (pthread_mutex_lock(&h->lock)) {
		OPAE_MSG("error locking handle mutex");
		return FPGA_EXCEPTION;
	}

	*value = *((volatile uint32_t *)get_user_offset(h, mmio_num, offset));
	pthread_mutex_unlock(&h->lock);

	return FPGA_OK;
}

static inline void copy512(const void *src, void *dst)
{
    asm volatile("vmovdqu64 (%0), %%zmm0;"
		 "vmovdqu64 %%zmm0, (%1);"
		 :
		 : "r"(src), "r"(dst));
}

fpga_result vfio_fpgaWriteMMIO512(fpga_handle handle,
				 uint32_t mmio_num,
				 uint64_t offset,
				 const void *value)
{
	vfio_handle *h = handle_check(handle);

	ASSERT_NOT_NULL(h);

	vfio_token *t = h->token;

	if (offset % 64 != 0) {
		OPAE_MSG("Misaligned MMIO access");
		return FPGA_INVALID_PARAM;
	}

	if (!(h->flags & OPAE_FLAG_HAS_AVX512)) {
		return FPGA_NOT_SUPPORTED;
	}

	if (t->hdr.objtype == FPGA_DEVICE)
		return FPGA_NOT_SUPPORTED;
	if (mmio_num > t->user_mmio_count)
		return FPGA_INVALID_PARAM;
	if (pthread_mutex_lock(&h->lock)) {
		OPAE_MSG("error locking handle mutex");
		return FPGA_EXCEPTION;
	}

	copy512(value, (uint8_t *)get_user_offset(h, mmio_num, offset));
	pthread_mutex_unlock(&h->lock);
	return FPGA_OK;
}

fpga_result vfio_fpgaMapMMIO(fpga_handle handle,
			     uint32_t mmio_num,
			     uint64_t **mmio_ptr)
{
	vfio_handle *h = handle_check(handle);

	ASSERT_NOT_NULL(h);

	vfio_token *t = h->token;

	if (mmio_num > t->user_mmio_count)
		return FPGA_INVALID_PARAM;

	/* Store return value only if return pointer has allocated memory */
	if (mmio_ptr)
		*mmio_ptr = (uint64_t *)get_user_offset(h, mmio_num, 0);

	return FPGA_OK;
}

fpga_result vfio_fpgaUnmapMMIO(fpga_handle handle,
			       uint32_t mmio_num)
{
	vfio_handle *h = handle_check(handle);

	ASSERT_NOT_NULL(h);

	vfio_token *t = h->token;

	if (mmio_num > t->user_mmio_count)
		return FPGA_INVALID_PARAM;
	return FPGA_OK;
}

void print_dfh(uint32_t offset, dfh *h)
{
	printf("0x%x: 0x%lx\n", offset, *(uint64_t *)h);
	printf("id: %d, rev: %d, next: %d, eol: %d, afu_minor: %d, version: %d, type: %d\n",
	       h->id, h->major_rev, h->next, h->eol, h->minor_rev, h->version, h->type);
}

vfio_token *find_token(const pci_device_t *p, uint32_t region)
{
	vfio_token *t = p->tokens;

	while (t) {
		if (t->region == region)
			return t;
		t = t->next;
	}
	return t;
}

vfio_token *get_token(pci_device_t *p, uint32_t region, int type)
{
	vfio_token *t = find_token(p, region);

	if (t)
		return t;
	t = (vfio_token *)malloc(sizeof(vfio_token));
	if (!t) {
		ERR("Failed to allocate memory for vfio_token");
		return NULL;
	}
	memset(t, 0, sizeof(vfio_token));
	t->hdr.magic = VFIO_TOKEN_MAGIC;
	t->device = p;
	t->region = region;
	t->hdr.objtype = type;
	t->next = p->tokens;
	p->tokens = t;
	return t;
}

bool pci_matches_filter(const fpga_properties *filter, pci_device_t *dev)
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
		 pci_device_t *dev)
{
	if (!filters)
		return true;
	for (uint32_t i = 0; i < num_filters; ++i) {
		if (pci_matches_filter(filters[i], dev))
			return true;
	}
	return false;
}

bool matches_filter(const fpga_properties *filter, vfio_token *t)
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

bool matches_filters(const fpga_properties *filters, uint32_t num_filters,
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

void dump_csr(uint8_t *begin, uint8_t *end, uint32_t index)
{
	char fname[PATH_MAX] = { 0 };
	char str_value[64] = { 0 };

	snprintf(fname, sizeof(fname), "csr_%d.dat", index);
	FILE *fp = fopen(fname, "w");

	if (!fp) {
		ERR("could not open file: %s", fname);
		return;
	}
	for (uint8_t *ptr = begin; ptr < end; ptr += 8) {
		uint64_t value = *(uint64_t *)ptr;

		if (value) {
			snprintf(str_value, sizeof(str_value), "0x%lx: 0x%lx\n", ptr-begin, value);
			fwrite(str_value, 1, strlen(str_value), fp);
		}
	}
	fclose(fp);

}

dfh *next_feature(dfh *h)
{
	if (!h->next)
		return NULL;
	if (!h->version) {
		dfh0 *h0 = (dfh0 *)((uint8_t *)h + h->next);

		while (h0->next && h0->type != 0x4)
			h0 = (dfh0 *)((uint8_t *)h0 + h0->next);
		return h0->next ? (dfh *)h0 : NULL;
	}
	return NULL;
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

fpga_result vfio_fpgaEnumerate(const fpga_properties *filters,
			       uint32_t num_filters, fpga_token *tokens,
			       uint32_t max_tokens, uint32_t *num_matches)
{
	pci_device_t *dev = _pci_devices;
	uint32_t matches = 0;

	while (dev) {
		if (pci_matches_filters(filters, num_filters, dev)) {
			vfio_walk(dev);
			vfio_token *ptr = dev->tokens;

			while (ptr) {
				vfio_pair_t *pair = NULL;
				fpga_result res;

				ptr->hdr.vendor_id = (uint16_t)ptr->device->vendor;
				ptr->hdr.device_id = (uint16_t)ptr->device->device;
				ptr->hdr.subsystem_vendor_id = ptr->device->subsystem_vendor;
				ptr->hdr.subsystem_device_id = ptr->device->subsystem_device;
				ptr->hdr.segment = ptr->device->bdf.segment;
				ptr->hdr.bus = ptr->device->bdf.bus;
				ptr->hdr.device = ptr->device->bdf.device;
				ptr->hdr.function = ptr->device->bdf.function;
				ptr->hdr.interface = FPGA_IFC_VFIO;
				//ptr->hdr.objtype = <already populated>
				ptr->hdr.object_id = ((uint64_t)ptr->device->bdf.bdf) << 32 | ptr->region;
				if (ptr->hdr.objtype == FPGA_DEVICE)
					memcpy(ptr->hdr.guid, ptr->compat_id, sizeof(fpga_guid));

				res = open_vfio_pair(ptr->device->addr, &pair);
				if (res == FPGA_OK) {
					ptr->num_afu_irqs = vfio_irq_count(pair->device);

					close_vfio_pair(&pair);
					ptr->afu_state = FPGA_ACCELERATOR_UNASSIGNED;
				} else {
					ptr->afu_state = FPGA_ACCELERATOR_ASSIGNED;
				}

				if (matches_filters(filters, num_filters, ptr)) {
					if (matches < max_tokens) {
						tokens[matches] =
							clone_token(ptr);
					}
					++matches;
				}
				ptr = ptr->next;
			}
		}
		dev = dev->next;
	}
	*num_matches = matches;
	return FPGA_OK;

}

fpga_result vfio_fpgaCloneToken(fpga_token src, fpga_token *dst)
{
	vfio_token *_src = (vfio_token *)src;
	vfio_token *_dst;

	if (!src || !dst) {
		OPAE_ERR("src or dst is NULL");
		return FPGA_INVALID_PARAM;
	}
	if (_src->hdr.magic != VFIO_TOKEN_MAGIC) {
		OPAE_ERR("Invalid src");
		return FPGA_INVALID_PARAM;
	}

	_dst = malloc(sizeof(vfio_token));
	if (!_dst) {
		ERR("Failed to allocate memory for vfio_token");
		return FPGA_NO_MEMORY;
	}
	memcpy(_dst, _src, sizeof(vfio_token));
	*dst = _dst;
	return FPGA_OK;
}

fpga_result vfio_fpgaDestroyToken(fpga_token *token)
{
	if (!token || !*token) {
		OPAE_ERR("invalid token pointer");
		return FPGA_INVALID_PARAM;
	}
	vfio_token *t = (vfio_token *)*token;

	if (t->hdr.magic == VFIO_TOKEN_MAGIC) {
		free(t);
		return FPGA_OK;
	}
	return FPGA_INVALID_PARAM;
}

#define HUGE_1G (1*1024*1024*1024)
#define HUGE_2M (2*1024*1024)
#define ROUND_UP(N, M) ((N + M - 1) & ~(M-1))

fpga_result vfio_fpgaPrepareBuffer(fpga_handle handle, uint64_t len,
				   void **buf_addr, uint64_t *wsid,
				   int flags)
{
	vfio_handle *h;
	uint8_t *virt = NULL;

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
	vfio_buffer *buffer = (vfio_buffer *)malloc(sizeof(vfio_buffer));

	if (!buffer) {
		OPAE_ERR("error allocating buffer metadata");
		if (opae_vfio_buffer_free(v, virt)) {
			OPAE_ERR("error freeing vfio buffer");
		}
		res = FPGA_NO_MEMORY;
		goto out_free;
	}
	memset(buffer, 0, sizeof(vfio_buffer));
	buffer->vfio_device = v;
	buffer->virtual = virt;
	buffer->iova = iova;
	buffer->size = sz;
	if (pthread_mutex_lock(&_buffers_mutex)) {
		OPAE_MSG("error locking buffer mutex");
		res = FPGA_EXCEPTION;
		goto out_free;
	}
	buffer->next = _vfio_buffers;
	buffer->wsid = buffer->next ? buffer->next->wsid+1 : 0;
	_vfio_buffers = buffer;
	*buf_addr = virt;
	*wsid = buffer->wsid;
	res = FPGA_OK;
	if (pthread_mutex_unlock(&_buffers_mutex)) {
		OPAE_MSG("error unlocking buffers");
		res = FPGA_EXCEPTION;
	}
out_free:
	if (res) {
		if (buffer) {
			free(buffer);
			if (virt && opae_vfio_buffer_free(v, virt)) {
				OPAE_ERR("error freeing vfio buffer");
			}
		}
	}
	return res;
}

fpga_result vfio_fpgaReleaseBuffer(fpga_handle handle, uint64_t wsid)
{
	vfio_handle *h = handle_check(handle);

	ASSERT_NOT_NULL(h);

	struct opae_vfio *v = h->vfio_pair->device;

	vfio_buffer *ptr = _vfio_buffers;
	vfio_buffer *prev = NULL;

	if (pthread_mutex_lock(&_buffers_mutex)) {
		OPAE_MSG("error locking buffer mutex");
		return FPGA_EXCEPTION;
	}
	fpga_result res = FPGA_NOT_FOUND;

	while (ptr) {
		if (ptr->wsid == wsid) {
			if (opae_vfio_buffer_free(v, ptr->virtual)) {
				OPAE_ERR("error freeing vfio buffer");
			}
			if (prev) {
				prev->next = ptr->next;
			} else {
				_vfio_buffers = ptr->next;
			}
			free(ptr);
			res = FPGA_OK;
			goto out_unlock;
		}
		prev = ptr;
		ptr = ptr->next;
	}
out_unlock:
	if (pthread_mutex_unlock(&_buffers_mutex)) {
		OPAE_MSG("error unlocking buffers mutex");
	}
	return res;
}

fpga_result vfio_fpgaGetIOAddress(fpga_handle handle, uint64_t wsid,
				  uint64_t *ioaddr)
{
	(void)handle;
	if (pthread_mutex_lock(&_buffers_mutex)) {
		OPAE_MSG("error locking buffer mutex");
		return FPGA_EXCEPTION;
	}
	vfio_buffer *ptr = _vfio_buffers;
	fpga_result res = FPGA_OK;

	while (ptr) {
		if (ptr->wsid == wsid) {
			*ioaddr = ptr->iova;
			goto out_unlock;
		}
		ptr = ptr->next;
	}
	res = FPGA_NOT_FOUND;
out_unlock:
	if (pthread_mutex_unlock(&_buffers_mutex)) {
		OPAE_MSG("error unlocking buffers mutex");
	}
	return res;
}

fpga_result vfio_fpgaCreateEventHandle(fpga_event_handle *event_handle)
{
	vfio_event_handle *_veh;
	fpga_result res = FPGA_OK;
	pthread_mutexattr_t mattr;
	int err;

	ASSERT_NOT_NULL(event_handle);

	_veh = malloc(sizeof(vfio_event_handle));
	if (!_veh) {
		OPAE_ERR("Out of memory");
		return FPGA_NO_MEMORY;
	}

	_veh->magic = VFIO_EVENT_HANDLE_MAGIC;

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
	free(_veh);
	return res;
}

fpga_result vfio_fpgaDestroyEventHandle(fpga_event_handle *event_handle)
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

	_veh->magic = ~_veh->magic;

	err = pthread_mutex_unlock(&_veh->lock);
	if (err)
		OPAE_ERR("pthread_mutex_unlock() failed: %s",
			 strerror(errno));

	err = pthread_mutex_destroy(&_veh->lock);
	if (err)
		OPAE_ERR("pthread_mutex_destroy() failed: %s",
			 strerror(errno));

	free(*event_handle);
	*event_handle = NULL;
	return FPGA_OK;
}

fpga_result vfio_fpgaGetOSObjectFromEventHandle(const fpga_event_handle eh,
						int *fd)
{
	vfio_event_handle *_veh;
	int err;

	ASSERT_NOT_NULL(eh);
	ASSERT_NOT_NULL(fd);

	_veh = event_handle_check_and_lock(eh);
	ASSERT_NOT_NULL(_veh);

	*fd = _veh->fd;

	err = pthread_mutex_unlock(&_veh->lock);
	if (err)
		OPAE_ERR("pthread_mutex_unlock() failed: %s",
			 strerror(errno));

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

fpga_result vfio_fpgaRegisterEvent(fpga_handle handle,
				   fpga_event_type event_type,
				   fpga_event_handle event_handle,
				   uint32_t flags)
{
	vfio_handle *_h;
	vfio_event_handle *_veh;
	fpga_result res = FPGA_OK;
	int err;

	ASSERT_NOT_NULL(handle);
	ASSERT_NOT_NULL(event_handle);

	_h = handle_check_and_lock(handle);
	ASSERT_NOT_NULL(_h);

	_veh = event_handle_check_and_lock(event_handle);
	if (!_veh)
		goto out_unlock_handle;

	res = register_event(_h, event_type, _veh, flags);

	err = pthread_mutex_unlock(&_veh->lock);
	if (err)
		OPAE_ERR("pthread_mutex_unlock() failed: %s",
			 strerror(errno));
out_unlock_handle:
	err = pthread_mutex_unlock(&_h->lock);
	if (err)
		OPAE_ERR("pthread_mutex_unlock() failed: %s",
			 strerror(errno));
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

fpga_result vfio_fpgaUnregisterEvent(fpga_handle handle,
				     fpga_event_type event_type,
				     fpga_event_handle event_handle)
{
	vfio_handle *_h;
	vfio_event_handle *_veh;
	fpga_result res = FPGA_OK;
	int err;

	ASSERT_NOT_NULL(handle);
	ASSERT_NOT_NULL(event_handle);

	_h = handle_check_and_lock(handle);
	ASSERT_NOT_NULL(_h);

	_veh = event_handle_check_and_lock(event_handle);
	if (!_veh)
		goto out_unlock_handle;

	res = unregister_event(_h, event_type, _veh);

	err = pthread_mutex_unlock(&_veh->lock);
	if (err)
		OPAE_ERR("pthread_mutex_unlock() failed: %s",
			 strerror(errno));
out_unlock_handle:
	err = pthread_mutex_unlock(&_h->lock);
	if (err)
		OPAE_ERR("pthread_mutex_unlock() failed: %s",
			 strerror(errno));
	return res;
}

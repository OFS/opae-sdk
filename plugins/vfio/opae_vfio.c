// Copyright(c) 2020, Intel Corporation
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

#undef _GNU_SOURCE
#include <opae/fpga.h>

#include "props.h"
#include "opae_vfio.h"
#include "dfl.h"

#define BAR_MAX 6
#define VFIO_TOKEN_MAGIC 0xEF1010FE
#define VFIO_HANDLE_MAGIC ~VFIO_TOKEN_MAGIC

#define PCIE_PATH_PATTERN "([0-9a-fA-F]{4}):([0-9a-fA-F]{2}):([0-9a-fA-F]{2})\\.([0-9])"
#define PCIE_PATH_PATTERN_GROUPS 5
#define PARSE_MATCH_INT(_p, _m, _v, _b, _l)                                    \
	do {                                                                   \
		char *endptr = NULL;                                           \
		_v = strtoul(_p + _m.rm_so, &endptr, _b);                      \
		if (!_v && endptr == _p + _m.rm_so) {                          \
			OPAE_MSG("error parsing int");                         \
			goto _l;                                               \
		}                                                              \
	} while (0)


const char * fme_drivers[] = {
	"bfaf2ae9-4a52-46e3-82fe-38f0f9e17764",
	0
};

typedef struct _vfio_buffer
{
	uint8_t *virtual;
	uint64_t iova;
	uint64_t wsid;
	size_t size;
	struct opae_vfio *vfio_device;
	struct _vfio_buffer *next;
} vfio_buffer;


static pci_device_t *_pci_devices;
static vfio_token *_vfio_tokens;
static vfio_buffer *_vfio_buffers;
static pthread_mutex_t _buffers_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

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
	if (res) return res;
	uint32_t v = strtoul(str_value, &endptr, 0);
	if (!v && str_value == endptr) {
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

void free_device_list()
{
	while (_pci_devices) {
		pci_device_t *p = _pci_devices;
		_pci_devices = _pci_devices->next;
		free(p);
	}
}

void free_token_list()
{
	while (_vfio_tokens) {
		vfio_token *t = _vfio_tokens;
		_vfio_tokens = _vfio_tokens->next;
		free(t);
	}
}

void free_buffer_list()
{
	vfio_buffer *ptr = _vfio_buffers;
	vfio_buffer *tmp = NULL;
	while (ptr) {
		tmp = ptr;
		ptr = tmp->next;
		if (opae_vfio_buffer_free(tmp->vfio_device, tmp->virtual)) {
			OPAE_ERR("error freeing vfio buffer");
		}
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
	pci_device_t *p = find_pci_device(addr);
	if (p) return p;
	p = (pci_device_t*)malloc(sizeof(pci_device_t));
	memset(p, 0, sizeof(pci_device_t));

	strncpy(p->addr, addr, PCIADDR_MAX-1);

	if(read_pci_attr_u32(addr, "vendor", &p->vendor)) {
		OPAE_ERR("error reading 'vendor' attribute: %s", addr);
		goto free;
	}

	if(read_pci_attr_u32(addr, "device", &p->device)) {
		OPAE_ERR("error reading 'device' attribute: %s", addr);
		goto free;
	}

	if(read_pci_attr_u32(addr, "numa_node", &p->numa_node)) {
		OPAE_ERR("error opening 'numa_node' attribute: %s", addr);
		goto free;
	}

	if(parse_pcie_info(p, addr)) {
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

int pci_discover()
{
	int res = 1;
	const char *gpattern = "/sys/bus/pci/drivers/vfio-pci/????:??:??.?";
	glob_t pg;
	int gres = glob(gpattern, 0, NULL, &pg);
	if (gres) {
		ERR("error looking in vfio-pci");
		return res;
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
	if (src->magic != VFIO_TOKEN_MAGIC) return NULL;
	vfio_token *token = (vfio_token *)malloc(sizeof(vfio_token));
	memcpy(token, src, sizeof(vfio_token));
	return token;
}

vfio_token *token_check(fpga_token token)
{
	ASSERT_NOT_NULL_RESULT(token, NULL);
	vfio_token *t = (vfio_token*)token;
	if (t->magic != VFIO_TOKEN_MAGIC) {
		OPAE_ERR("invalid token magic");
		return NULL;
	}
	return t;
}

vfio_handle *handle_check(fpga_handle handle)
{
	ASSERT_NOT_NULL_RESULT(handle, NULL);
	vfio_handle *h = (vfio_handle*)handle;
	if (h->magic != VFIO_HANDLE_MAGIC) {
		OPAE_ERR("invalid handle magic");
		return NULL;
	}
	return h;
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

int walk(pci_device_t *p, int region)
{
	int res = 0;
	volatile uint8_t *mmio;
	size_t size;
	struct opae_vfio v;

	if (opae_vfio_open(&v, p->addr)) {
		OPAE_ERR("error opening vfio device: %s", p->addr);
		return 1;
	}

	// look for legacy FME guids in BAR 0
	if (opae_vfio_region_get(&v, 0, (uint8_t**)&mmio, &size)) {
		OPAE_ERR("error getting BAR%d", region);
		return 1;
	}

	// get the GUID at offset 0x8
	fpga_guid b0_guid;
	res = get_guid(((uint64_t*)mmio)+1, b0_guid);
	if (res) {
		OPAE_ERR("error reading guid");
		return 1;
	}

	// walk our known list of FME guids
	// and compare each one to the one read into b0_guid
	for (const char **u = fme_drivers; *u; u++) {
		fpga_guid uuid;
		res = uuid_parse(*u, uuid);
		if (res) {
			OPAE_ERR("error parsing uuid: %s", *u);
			return 1;
		}
		if (!uuid_compare(uuid, b0_guid)) {
			// we found a legacy FME in BAR0, walk it
			res = walk_fme(p, &v, mmio, 0);
			goto close;
		}
	}

	// treat all of BAR0 as an FPGA_ACCELERATOR
	vfio_token *t = get_token(p, 0, FPGA_ACCELERATOR);
	t->mmio_size = size;
	t->user_mmio_count = 1;
	t->user_mmio[0] = 0;

	// now let's check other BARs
	for (uint32_t i = 1; i < BAR_MAX; ++i) {
		if (!opae_vfio_region_get(&v, i, (uint8_t**)&mmio, &size)) {
			vfio_token *t = get_token(p, i, FPGA_ACCELERATOR);
			t->mmio_size = size;
			t->user_mmio_count = 1;
			t->user_mmio[0] = 0;
		}
	}

close:
	opae_vfio_close(&v);
	return res;
}

int features_discover()
{
	pci_device_t *p = _pci_devices;
	while(p) {
		walk(p, 0);
		p = p->next;
	}

	return 0;
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
	if (NULL == _handle) {
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
	if (opae_vfio_open(&_handle->vfio_device, _token->device->addr)) {
		OPAE_ERR("error opening vfio device");
		res = FPGA_EXCEPTION;
		goto out_attr_destroy;
	}
	uint8_t *mmio = NULL;
	size_t size;
	if (opae_vfio_region_get(&_handle->vfio_device,
				 _token->region, &mmio, &size)) {
		OPAE_ERR("error opening vfio region");
		res = FPGA_EXCEPTION;
		goto out_attr_destroy;
	}
	_handle->mmio_base = (volatile uint8_t*)(mmio);
	_handle->mmio_size = size;
	*handle = _handle;
	res = FPGA_OK;
out_attr_destroy:
	pthread_mutexattr_destroy(&mattr);
	if (res && _handle)
		free(_handle);
	return res;
}

fpga_result vfio_fpgaClose(fpga_handle handle)
{
	fpga_result res = FPGA_OK;
	vfio_handle *h = handle_check_and_lock(handle);
	ASSERT_NOT_NULL(h);

	if (token_check(h->token)) free(h->token);
	else OPAE_MSG("invalid token in handle");

	opae_vfio_close(&h->vfio_device);
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
	if (t->type == FPGA_ACCELERATOR && t->ops.reset) {
		res = t->ops.reset(t->device, h->mmio_base);
	}
	pthread_mutex_unlock(&h->lock);
	return res;
}

fpga_result get_guid(uint64_t* h, fpga_guid guid)
{
	ASSERT_NOT_NULL(h);
	size_t sz = 16;
	uint8_t *ptr = ((uint8_t*)h)+sz;
	for (size_t i = 0; i < sz; ++i) {
		guid[i] = *--ptr;
	}
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
	_prop->vendor_id = t->device->vendor;
	SET_FIELD_VALID(_prop, FPGA_PROPERTY_VENDORID);

	_prop->device_id = t->device->device;
	SET_FIELD_VALID(_prop, FPGA_PROPERTY_DEVICEID);

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


	_prop->objtype = t->type;
	SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);

	if (t->type == FPGA_ACCELERATOR) {
		if (t->parent) {
			_prop->parent = clone_token(t->parent);
			SET_FIELD_VALID(_prop, FPGA_PROPERTY_PARENT);
		}
		memcpy(_prop->guid, t->guid, sizeof(fpga_guid));
		SET_FIELD_VALID(_prop, FPGA_PROPERTY_GUID);

		_prop->u.accelerator.num_mmio = t->user_mmio_count;
		SET_FIELD_VALID(_prop, FPGA_PROPERTY_NUM_MMIO);
	} else {
		memcpy(_prop->guid, t->compat_id, sizeof(fpga_guid));
		SET_FIELD_VALID(_prop, FPGA_PROPERTY_GUID);

		_prop->u.fpga.bbs_id = t->bitstream_id;
		SET_FIELD_VALID(_prop, FPGA_PROPERTY_BBSID);

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
	if (result) return result;
	if (token) {
		result = vfio_fpgaUpdateProperties(token, _prop);
		if (result) goto out_free;
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

static inline volatile uint8_t *get_user_offset(vfio_handle * h,
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
	if (t->type == FPGA_DEVICE)
		return FPGA_NOT_SUPPORTED;
	if (mmio_num > t->user_mmio_count)
		return FPGA_INVALID_PARAM;
	if (pthread_mutex_lock(&h->lock)) {
		OPAE_MSG("error locking handle mutex");
		return FPGA_EXCEPTION;
	}

	*((volatile uint64_t*)get_user_offset(h, mmio_num, offset)) = value;
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
	if (t->type == FPGA_DEVICE)
		return FPGA_NOT_SUPPORTED;
	if (mmio_num > t->user_mmio_count)
		return FPGA_INVALID_PARAM;
	if (pthread_mutex_lock(&h->lock)) {
		OPAE_MSG("error locking handle mutex");
		return FPGA_EXCEPTION;
	}

	*value = *((volatile uint64_t*)get_user_offset(h, mmio_num, offset));
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
	if (t->type == FPGA_DEVICE)
		return FPGA_NOT_SUPPORTED;
	if (mmio_num > t->user_mmio_count)
		return FPGA_INVALID_PARAM;
	if (pthread_mutex_lock(&h->lock)) {
		OPAE_MSG("error locking handle mutex");
		return FPGA_EXCEPTION;
	}

	*((volatile uint32_t*)get_user_offset(h, mmio_num, offset)) = value;
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
	if (t->type == FPGA_DEVICE)
		return FPGA_NOT_SUPPORTED;
	if (mmio_num > t->user_mmio_count)
		return FPGA_INVALID_PARAM;
	if (pthread_mutex_lock(&h->lock)) {
		OPAE_MSG("error locking handle mutex");
		return FPGA_EXCEPTION;
	}

	*value = *((volatile uint32_t*)get_user_offset(h, mmio_num, offset));
	pthread_mutex_unlock(&h->lock);

	return FPGA_OK;
}

fpga_result vfio_fpgaMapMMIO(fpga_handle handle,
			     uint32_t mmio_num,
			     uint64_t **mmio_ptr)
{
	vfio_handle *h = handle_check(handle);
	ASSERT_NOT_NULL(h);

	vfio_token* t = h->token;
	if (mmio_num > t->user_mmio_count)
		return FPGA_INVALID_PARAM;
	*mmio_ptr = (uint64_t*)get_user_offset(h, mmio_num, 0);
	return FPGA_OK;
}

fpga_result vfio_fpgaUnmapMMIO(fpga_handle handle,
			       uint32_t mmio_num)
{
	vfio_handle *h = handle_check(handle);
	ASSERT_NOT_NULL(h);

	vfio_token* t = h->token;
	if (mmio_num > t->user_mmio_count)
		return FPGA_INVALID_PARAM;
	return FPGA_OK;
}

void print_dfh(uint32_t offset, dfh *h)
{
	printf("0x%x: 0x%lx\n", offset, *(uint64_t*)h);
	printf("id: %d, rev: %d, next: %d, eol: %d, afu_minor: %d, version: %d, type: %d\n",
               h->id, h->major_rev, h->next, h->eol, h->minor_rev, h->version, h->type);
}

vfio_token *find_token(const pci_device_t *p, uint32_t region)
{
	vfio_token *t = _vfio_tokens;
	while (t) {
		if (t->region == region &&
		    !strncmp(t->device->addr, p->addr, PCIADDR_MAX))
			return t;
		t = t->next;
	}
	return t;
}

vfio_token *get_token(const pci_device_t *p, uint32_t region, int type)
{
	vfio_token *t = find_token(p, region);
	if (t) return t;
	t = (vfio_token*)malloc(sizeof(vfio_token));
	memset(t, 0, sizeof(vfio_token));
	t->magic = VFIO_TOKEN_MAGIC;
	t->device = p;
	t->region = region;
	t->type = type;
	t->next = _vfio_tokens;
	_vfio_tokens = t;
	return t;
}


bool matches_filter(const fpga_properties *filter, vfio_token *t)
{
	struct _fpga_properties *_prop = (struct _fpga_properties *)filter;
	if (FIELD_VALID(_prop, FPGA_PROPERTY_PARENT)) {
		if (t->type == FPGA_DEVICE) return false;
		vfio_token *t_parent = (vfio_token*)t->parent;
		vfio_token *f_parent = (vfio_token*)_prop->parent;
		if (!t_parent) return false;
		if (t_parent->device->bdf.bdf != f_parent->device->bdf.bdf)
			return false;
		if (t_parent->region != f_parent->region) return false;
	}

	if (FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE))
		if (_prop->objtype != t->type) return false;
	if (FIELD_VALID(_prop, FPGA_PROPERTY_SEGMENT))
		if (_prop->segment != t->device->bdf.segment) return false;
	if (FIELD_VALID(_prop, FPGA_PROPERTY_BUS))
		if (_prop->bus != t->device->bdf.bus) return false;
	if (FIELD_VALID(_prop, FPGA_PROPERTY_DEVICE))
		if (_prop->device != t->device->bdf.device) return false;
	if (FIELD_VALID(_prop, FPGA_PROPERTY_FUNCTION))
		if (_prop->function != t->device->bdf.function) return false;
	if (FIELD_VALID(_prop, FPGA_PROPERTY_SOCKETID))
		if (_prop->socket_id != t->device->numa_node) return false;
	if (FIELD_VALID(_prop, FPGA_PROPERTY_GUID)) {
		if (memcmp(_prop->guid, t->guid, sizeof(fpga_guid))) return false;
	}
	return true;
}

bool matches_filters(const fpga_properties *filters, uint32_t num_filters,
		     vfio_token *t)
{
	if (!filters) return true;
	for (uint32_t i = 0; i < num_filters; ++i) {
		if (!matches_filter(filters[i], t)) return false;
	}
	return true;
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
	for (uint8_t *ptr = begin; ptr < end; ptr+=8) {
		uint64_t value = *(uint64_t*)ptr;
		if (value) {
			snprintf(str_value, sizeof(str_value), "0x%lx: 0x%lx\n", ptr-begin, value);
			fwrite(str_value, 1, strlen(str_value), fp);
		}
	}
	fclose(fp);

}

dfh *next_feature(dfh *h)
{
	if (!h->next) return NULL;
	if (!h->version) {
		dfh0 *h0 = (dfh0*)((uint8_t*)h + h->next);
		while(h0->next && h0->type != 0x4)
			h0 = (dfh0*)((uint8_t*)h0 + h0->next);
		return h0->next ? (dfh*)h0 : NULL;
	}
	return NULL;
}

fpga_result vfio_fpgaEnumerate(const fpga_properties *filters,
			       uint32_t num_filters, fpga_token *tokens,
			       uint32_t max_tokens, uint32_t *num_matches)
{
	vfio_token *ptr = _vfio_tokens;
	uint32_t matches = 0;
	while(ptr) {
		if (matches_filters(filters, num_filters, ptr)) {
			if (matches < max_tokens) {
				vfio_token *tok = clone_token(ptr);
				if (ptr->parent)
					tok->parent = clone_token(tok->parent);
				tokens[matches] = tok;
			}
			++matches;
		}
		ptr = ptr->next;
	}
	*num_matches = matches;
	return FPGA_OK;

}

fpga_result vfio_fpgaCloneToken(fpga_token src, fpga_token *dst)
{
	vfio_token *_src = (vfio_token*)src;
	vfio_token *_dst;
	if (!src || !dst) {
		OPAE_ERR("src or dst is NULL");
		return FPGA_INVALID_PARAM;
	}
	if (_src->magic != VFIO_TOKEN_MAGIC) {
		OPAE_ERR("Invalid src");
		return FPGA_INVALID_PARAM;
	}

	_dst = malloc(sizeof(vfio_token));
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
	vfio_token *t = (vfio_token*)*token;
	if (t->magic == VFIO_TOKEN_MAGIC) {
		free(t);
		return FPGA_OK;
	}
	return FPGA_INVALID_PARAM;
}

#define HUGE_1G 1*1024*1024*1024
#define HUGE_2M 2*1024*1024
#define ROUND_UP(N, M) (N + M - 1) & -M

fpga_result vfio_fpgaPrepareBuffer(fpga_handle handle, uint64_t len,
				   void **buf_addr, uint64_t *wsid,
				   int flags)
{
	ASSERT_NOT_NULL(buf_addr);
	ASSERT_NOT_NULL(wsid);
	vfio_handle *h = handle_check(handle);
	ASSERT_NOT_NULL(h);

	(void)flags;
	fpga_result res = FPGA_EXCEPTION;

	struct opae_vfio *v = (struct opae_vfio*)&h->vfio_device;
	uint8_t *virt = NULL;
	uint64_t iova = 0;
	size_t sz = len > HUGE_2M ? ROUND_UP(len, HUGE_1G) :
		    len > 4096 ? ROUND_UP(len, HUGE_2M) : 4096;
	if (opae_vfio_buffer_allocate(v, &sz, &virt, &iova)) {
		OPAE_ERR("could not allocate buffer");
		return FPGA_EXCEPTION;
	}
	vfio_buffer *buffer = (vfio_buffer*)malloc(sizeof(vfio_buffer));
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

	struct opae_vfio *v = (struct opae_vfio*)&h->vfio_device;

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
	while (ptr) {
		if (ptr->wsid == wsid) {
			*ioaddr = ptr->iova;
			return FPGA_OK;
		}
		ptr = ptr->next;
	}

	return FPGA_NOT_FOUND;
}


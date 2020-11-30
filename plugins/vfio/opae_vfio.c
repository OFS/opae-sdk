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
#include <sys/stat.h>

#undef _GNU_SOURCE
#include <opae/fpga.h>

#include "props.h"
#include "opae_vfio.h"
#include "dfl.h"

#define VFIO_TOKEN_MAGIC 0xEF1010FE
#define VFIO_HANDLE_MAGIC ~VFIO_TOKEN_MAGIC

#define PCIE_PATH_PATTERN "([0-9a-fA-F]{4}):([0-9a-fA-F]{2}):([0-9a-fA-F]{2})\\.([0-9])"
#define PCIE_PATH_PATTERN_GROUPS 5
#define PARSE_MATCH_INT(_p, _m, _v, _b, _l)                                    \
	do {                                                                   \
		errno = 0;                                                     \
		_v = strtoul(_p + _m.rm_so, NULL, _b);                         \
		if (errno) {                                                   \
			OPAE_MSG("error parsing int");                         \
			goto _l;                                               \
		}                                                              \
	} while (0)


#ifndef __FILENAME__
#define SLASHPTR strrchr(__FILE__, '/')
#define __FILENAME__ (SLASHPTR ? SLASHPTR+1 : __FILE__)
#endif

#define ERR(format, ...)                                                       \
	fprintf(stderr, "%s:%u:%s() [ERROR][%s] : " format,                    \
	__FILENAME__, __LINE__, __func__, strerror(errno), ##__VA_ARGS__)

#define FME_GUID_LO 0x82FE38F0F9E17764
#define FME_GUID_HI 0xBFAF2AE94A5246E3

typedef struct _driver
{
	uint64_t guid_lo;
	uint64_t guid_hi;
} driver;

driver fme_drivers[] = {
	{FME_GUID_LO, FME_GUID_HI}
};

typedef struct _vfio_buffer
{
	uint8_t *virtual;
	uint64_t iova;
	uint64_t wsid;
	size_t size;
	struct _vfio_buffer *next;
} vfio_buffer;


static pci_device_t *_pci_devices;
static vfio_token *_vfio_tokens;
static vfio_buffer *_vfio_buffers;

static int read_pci_attr(const char *addr, const char *attr, char *value, size_t max)
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

static int read_pci_attr_u32(const char *addr, const char *attr, uint32_t *value)
{
	char str_value[64];
	int res = read_pci_attr(addr, attr, str_value, sizeof(str_value));
	if (res) return res;
	errno = 0;
	*value = strtoul(str_value, NULL, 0);
	if (errno) {
		ERR("error parsing string: %s", str_value);
		return FPGA_EXCEPTION;
	}
	return FPGA_OK;
}

static int parse_pcie_info(pci_device_t *device, char *addr)
{
	char err[128] = {0};
	regex_t re;
	regmatch_t matches[PCIE_PATH_PATTERN_GROUPS] = { {0} };
	int res = FPGA_EXCEPTION;

	int reg_res = regcomp(&re, PCIE_PATH_PATTERN, REG_EXTENDED | REG_ICASE);
	if (reg_res) {
		OPAE_ERR("Error compling regex");
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
		ERR("error reading 'vendor' attribute: %s", addr);
		goto free;
	}

	if(read_pci_attr_u32(addr, "device", &p->device)) {
		ERR("error reading 'device' attribute: %s", addr);
		goto free;
	}

	if(read_pci_attr_u32(addr, "numa_node", &p->numa_node)) {
		ERR("error opening 'numa_node' attribute: %s", addr);
		goto free;
	}

	if(parse_pcie_info(p, addr)) {
		ERR("error parsing pcie address: %s", addr);
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
			printf("error with gl_pathv\n");
			continue;
		}

		pci_device_t *dev = get_pci_device(p + 1);
		if (!dev) {
			ERR("error with pci address: %s", p + 1);
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
	if (src->magic != VFIO_TOKEN_MAGIC) return NULL;
	vfio_token *token = (vfio_token *)malloc(sizeof(vfio_token));
	memcpy(token, src, sizeof(vfio_token));
	return token;
}

fpga_result token_check(fpga_token token)
{
	vfio_token *t = (vfio_token*)token;
	if (t->magic != VFIO_TOKEN_MAGIC) {
		printf("invalid token magic\n");
		return FPGA_INVALID_PARAM;
	}
	return FPGA_OK;
}

fpga_result handle_check(fpga_handle handle)
{
	vfio_handle *h = (vfio_handle*)handle;
	if (h->magic != VFIO_HANDLE_MAGIC) {
		printf("invalid handle magic\n");
		return FPGA_INVALID_PARAM;
	}
	return FPGA_OK;
}

int walk(pci_device_t *p, int region)
{
	int res = 0;
	volatile uint8_t *mmio;
	size_t size;
	struct opae_vfio v;

	if (opae_vfio_open(&v, p->addr)) {
		ERR("error opening vfio device: %s", p->addr);
		return 1;
	}
	if (opae_vfio_region_get(&v, 0, (uint8_t**)&mmio, &size)) {
		ERR("error getting BAR%d", region);
		return 1;
	}

	volatile uint64_t *lo = ((uint64_t*)mmio)+1;
	volatile uint64_t *hi = lo+1;
	for (size_t i = 0; i < sizeof(fme_drivers); ++i) {
		if (*lo == fme_drivers[i].guid_lo &&
		    *hi == fme_drivers[i].guid_hi) {
			res = walk_fme(p, &v, mmio, 0);
			goto close;
		}
	}
	for (uint32_t i = 0; i < 8; ++i) {
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
	vfio_token *_token;
	vfio_handle *_handle;

	if (NULL == token) {
		printf("token is NULL");
		return FPGA_INVALID_PARAM;
	}

	if (NULL == handle) {
		printf("handle is NULL");
		return FPGA_INVALID_PARAM;
	}

	if (flags & FPGA_OPEN_SHARED) {
		printf("shared mode ignored at this time\n");
		//return FPGA_INVALID_PARAM;
	}

	int res = token_check(token);
	if (res) return res;

	_token = (vfio_token *)token;

	_handle = malloc(sizeof(vfio_handle));
	if (NULL == _handle) {
		printf("Failed to allocate memory for handle");
		return FPGA_NO_MEMORY;
	}

	memset(_handle, 0, sizeof(*_handle));

	// mark data structure as valid
	_handle->magic = VFIO_HANDLE_MAGIC;
	_handle->token = clone_token(_token);
	if (opae_vfio_open(&_handle->vfio_device, _token->device->addr)) {
		printf("error opening vfio device\n");
		free(_handle);
		return FPGA_EXCEPTION;
	}
	uint8_t *mmio = NULL;
	size_t size;
	if (opae_vfio_region_get(&_handle->vfio_device,
				 _token->region, &mmio, &size)) {
		printf("error opening vfio region\n");
		free(_handle);
		return FPGA_EXCEPTION;
	}
	_handle->mmio_base = (volatile uint8_t*)(mmio);
	_handle->mmio_size = size;
	*handle = _handle;
	return FPGA_OK;
}

fpga_result vfio_fpgaClose(fpga_handle handle)
{
	int res = handle_check(handle);
	if (res) return res;

	vfio_handle *h = (vfio_handle*)handle;
	res = token_check(h->token);
	if (res) return res;
	opae_vfio_close(&h->vfio_device);
	free(h->token);
	free(h);
	return FPGA_OK;
}

fpga_result vfio_fpgaReset(fpga_handle handle)
{
	fpga_result res = handle_check(handle);
	if (res) return res;

	vfio_handle *h = (vfio_handle*)handle;
	vfio_token *t = h->token;
	if (t->type == FPGA_DEVICE)
		return FPGA_NOT_SUPPORTED;
	if (t->ops.reset) {
		return t->ops.reset(t->device, h->mmio_base);
	}
	return FPGA_NOT_SUPPORTED;
}

fpga_result get_guid(uint64_t* h, fpga_guid guid)
{
	size_t sz = 16;
	uint8_t *ptr = ((uint8_t*)h)+sz;
	for (size_t i = 0; i < sz; ++i) {
		guid[i] = *--ptr;
	}
	return FPGA_OK;
}

fpga_result vfio_fpgaUpdateProperties(fpga_token token, fpga_properties prop)
{
	fpga_result res = token_check(token);
	if (res) return res;

	vfio_token *t = (vfio_token*)token;

	struct _fpga_properties *_prop = (struct _fpga_properties *)prop;
	if (!_prop) {
		return FPGA_EXCEPTION;
	}
	if (_prop->magic != FPGA_PROPERTY_MAGIC) {
		printf("Invalid properties object");
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
	(void)token;
	(void)prop;
	struct _fpga_properties *_prop = NULL;
	fpga_result result = FPGA_OK;

	//ASSERT_NOT_NULL(prop);

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
	fpga_result res = handle_check(handle);
	if (res) return res;

	vfio_handle *h = (vfio_handle*)handle;
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
	fpga_result res = handle_check(handle);
	if (res) return res;

	vfio_handle *h = (vfio_handle*)handle;
	vfio_token *t = h->token;
	if (t->type == FPGA_DEVICE)
		return FPGA_NOT_SUPPORTED;
	if (mmio_num > t->user_mmio_count)
		return FPGA_INVALID_PARAM;

	*((uint64_t*)get_user_offset(h, mmio_num, offset)) = value;
	return FPGA_OK;
}

fpga_result vfio_fpgaReadMMIO64(fpga_handle handle,
				uint32_t mmio_num,
				uint64_t offset,
				uint64_t *value)
{
	fpga_result res = handle_check(handle);
	if (res) return res;
	vfio_handle *h = (vfio_handle*)handle;
	vfio_token *t = h->token;
	if (t->type == FPGA_DEVICE)
		return FPGA_NOT_SUPPORTED;
	if (mmio_num > t->user_mmio_count)
		return FPGA_INVALID_PARAM;

	*value = *((uint64_t*)get_user_offset(h, mmio_num, offset));
	return FPGA_OK;
}

fpga_result vfio_fpgaWriteMMIO32(fpga_handle handle,
				 uint32_t mmio_num,
				 uint64_t offset,
				 uint32_t value)
{
	fpga_result res = handle_check(handle);
	if (res) return res;

	vfio_handle *h = (vfio_handle*)handle;
	vfio_token *t = h->token;
	if (t->type == FPGA_DEVICE)
		return FPGA_NOT_SUPPORTED;
	if (mmio_num > t->user_mmio_count)
		return FPGA_INVALID_PARAM;
	*((uint32_t*)get_user_offset(h, mmio_num, offset)) = value;
	return FPGA_OK;
}

fpga_result vfio_fpgaReadMMIO32(fpga_handle handle,
				uint32_t mmio_num,
				uint64_t offset,
				uint32_t *value)
{
	fpga_result res = handle_check(handle);
	if (res) return res;

	vfio_handle *h = (vfio_handle*)handle;
	vfio_token *t = h->token;
	if (t->type == FPGA_DEVICE)
		return FPGA_NOT_SUPPORTED;
	if (mmio_num > t->user_mmio_count)
		return FPGA_INVALID_PARAM;
	*value = *((uint32_t*)get_user_offset(h, mmio_num, offset));
	return FPGA_OK;
}

fpga_result vfio_fpgaMapMMIO(fpga_handle handle,
			     uint32_t mmio_num,
			     uint64_t **mmio_ptr)
{
	fpga_result res = handle_check(handle);
	if (res) return res;
	vfio_handle *h = (vfio_handle*)handle;
	vfio_token* t = h->token;
	if (mmio_num > t->user_mmio_count)
		return FPGA_INVALID_PARAM;
	*mmio_ptr = (uint64_t*)get_user_offset(h, mmio_num, 0);
	return FPGA_OK;
}

fpga_result vfio_fpgaUnmapMMIO(fpga_handle handle,
			       uint32_t mmio_num)
{
	fpga_result res = handle_check(handle);
	if (res) return res;
	vfio_handle *h = (vfio_handle*)handle;
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
		printf("src or dst is NULL\n");
		return FPGA_INVALID_PARAM;
	}
	if (_src->magic != VFIO_TOKEN_MAGIC) {
		printf("Invalid src\n");
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
		printf("invalid token pointer\n");
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
	fpga_result res = handle_check(handle);
	if (res) return res;

	(void)flags;
	vfio_handle *h = (vfio_handle*)handle;
	struct opae_vfio *v = (struct opae_vfio*)&h->vfio_device;
	uint8_t *virt = NULL;
	uint64_t iova = 0;
	size_t sz = len > HUGE_2M ? ROUND_UP(len, HUGE_1G) :
		    len > 4096 ? ROUND_UP(len, HUGE_2M) : 4096;
	if (opae_vfio_buffer_allocate(v, &sz, &virt, &iova)) {
		printf("could not allocate buffer\n");
		return FPGA_EXCEPTION;
	}
	vfio_buffer *buffer = (vfio_buffer*)malloc(sizeof(vfio_buffer));
	if (!buffer) {
		printf("error allocating buffer metadata\n");
		if (opae_vfio_buffer_free(v, virt)) {
			printf("error freeing vfio buffer\n");
		}
		return FPGA_NO_MEMORY;
	}
	memset(buffer, 0, sizeof(vfio_buffer));

	if (_vfio_buffers) {
		buffer->wsid = _vfio_buffers->wsid+1;
	}
	buffer->virtual = virt;
	buffer->iova = iova;
	buffer->size = sz;
	buffer->next = _vfio_buffers;
	_vfio_buffers = buffer;
	*buf_addr = virt;
	*wsid = buffer->wsid;
	return FPGA_OK;
}

fpga_result vfio_fpgaReleaseBuffer(fpga_handle handle, uint64_t wsid)
{
	fpga_result res = handle_check(handle);
	if (res) return res;

	vfio_handle *h = (vfio_handle*)handle;
	struct opae_vfio *v = (struct opae_vfio*)&h->vfio_device;

	vfio_buffer *ptr = _vfio_buffers;
	vfio_buffer *prev = NULL;
	while (ptr) {
		if (ptr->wsid == wsid) {
			if (opae_vfio_buffer_free(v, ptr->virtual)) {
				printf("error freeing vfio buffer\n");
			}
			if (prev) {
				prev->next = ptr->next;
			} else {
				_vfio_buffers = ptr->next;
			}
			free(ptr);
			return FPGA_OK;
		}
		prev = ptr;
		ptr = ptr->next;
	}

	return FPGA_NOT_FOUND;
}

fpga_result vfio_fpgaGetIOAddress(fpga_handle handle, uint64_t wsid,
			          uint64_t *ioaddr)
{
	(void)handle;
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


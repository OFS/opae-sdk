// Copyright(c) 2018, Intel Corporation
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

#ifndef __OPAE_PROPS_H__
#define __OPAE_PROPS_H__

#include <stdint.h>
#ifndef __USE_GNU
#define __USE_GNU 1
#endif
#include <pthread.h>

#include <opae/types.h>
#include <opae/types_enum.h>

#include "opae_int.h"

// FPGA property magic (FPGAPROP)
#define FPGA_PROPERTY_MAGIC 0x4650474150524f50

/** Fields common across all object types */
#define FPGA_PROPERTY_PARENT         0
#define FPGA_PROPERTY_OBJTYPE        1
#define FPGA_PROPERTY_SEGMENT        2
#define FPGA_PROPERTY_BUS            3
#define FPGA_PROPERTY_DEVICE         4
#define FPGA_PROPERTY_FUNCTION       5
#define FPGA_PROPERTY_SOCKETID       6
#define FPGA_PROPERTY_VENDORID       7
#define FPGA_PROPERTY_DEVICEID       8
#define FPGA_PROPERTY_GUID           9
#define FPGA_PROPERTY_OBJECTID       10
#define FPGA_PROPERTY_NUM_ERRORS     11

/** Fields for FPGA objects */
#define FPGA_PROPERTY_NUM_SLOTS     32
#define FPGA_PROPERTY_BBSID         33
#define FPGA_PROPERTY_BBSVERSION    34
#define FPGA_PROPERTY_MODEL         35
#define FPGA_PROPERTY_LOCAL_MEMORY  36
#define FPGA_PROPERTY_CAPABILITIES  37

/** Fields for accelerator objects */
#define FPGA_PROPERTY_ACCELERATOR_STATE 32
#define FPGA_PROPERTY_NUM_MMIO          33
#define FPGA_PROPERTY_NUM_INTERRUPTS    34


#define FIELD_VALID(P, F) (((P)->valid_fields >> (F)) & 1)

#define SET_FIELD_VALID(P, F)\
	((P)->valid_fields = (P)->valid_fields | ((uint64_t)1 << (F)))

#define CLEAR_FIELD_VALID(P, F)\
	((P)->valid_fields = (P)->valid_fields & ~((uint64_t)1 << (F)))


struct _fpga_properties {
	pthread_mutex_t lock;
	uint64_t magic;
	/* Common properties */
	uint64_t valid_fields; // bitmap of valid fields
	// valid here means the field has been set using the API
	// bit 0x00 - parent field is valid
	// bit 0x01 - objtype field is valid
	// bit 0x02 - segment field is valid
	// ...
	// up to bit 0x1F
	fpga_guid guid;		// Applies only to accelerator types
	fpga_token parent;
	fpga_objtype objtype;
	uint16_t segment;
	uint8_t bus;
	uint8_t device;
	uint8_t function;
	uint8_t socket_id;
	uint64_t object_id;
	uint16_t vendor_id;
	uint16_t device_id;
	uint32_t num_errors;

	/* Object-specific properties
	 * bitfields start as 0x20
	 */
	union {

		/* fpga object properties
		 * */
		struct {
			uint32_t num_slots;
			uint64_t bbs_id;
			fpga_version bbs_version;
			// TODO char model[FPGA_MODEL_LENGTH];
			// TODO uint64_t local_memory_size;
			// TODO uint64_t capabilities; #<{(| bitfield (HSSI, iommu, ...) |)}>#
		} fpga;

		/* accelerator object properties
		 * */
		struct {
			fpga_accelerator_state state;
			uint32_t num_mmio;
			uint32_t num_interrupts;
		} accelerator;

	} u;

};

// returns NULL on error, locked _fpga_properties object on success.
static inline struct _fpga_properties *
 opae_validate_and_lock_properties(fpga_properties props)
{
	int res;
	struct _fpga_properties *p = (struct _fpga_properties *) props;

	if (!p)
		return NULL;

	opae_mutex_lock(res, &p->lock);

	if (p->magic != FPGA_PROPERTY_MAGIC) {
		opae_mutex_unlock(res, &p->lock);
		return NULL;
	}

	return p;
}

struct _fpga_properties * opae_properties_create(void);

void opae_properties_destroy(struct _fpga_properties *props);

struct _fpga_properties * opae_properties_clone(fpga_properties props);

fpga_result opae_properties_clear(fpga_properties props);

fpga_result opae_properties_get_parent(fpga_properties props, fpga_token *parent);
fpga_result opae_properties_set_parent(fpga_properties props, fpga_token parent);

fpga_result opae_properties_get_object_type(fpga_properties props, fpga_objtype *objtype);
fpga_result opae_properties_set_object_type(fpga_properties props, fpga_objtype objtype);

fpga_result opae_properties_get_segment(fpga_properties props, uint16_t *segment);
fpga_result opae_properties_set_segment(fpga_properties props, uint16_t segment);

fpga_result opae_properties_get_bus(fpga_properties props, uint8_t *bus);
fpga_result opae_properties_set_bus(fpga_properties props, uint8_t bus);

fpga_result opae_properties_get_device(fpga_properties props, uint8_t *device);
fpga_result opae_properties_set_device(fpga_properties props, uint8_t device);

fpga_result opae_properties_get_function(fpga_properties props, uint8_t *function);
fpga_result opae_properties_set_function(fpga_properties props, uint8_t function);

fpga_result opae_properties_get_socket_id(fpga_properties props, uint8_t *socket_id);
fpga_result opae_properties_set_socket_id(fpga_properties props, uint8_t socket_id);

fpga_result opae_properties_get_device_id(fpga_properties props, uint16_t *device_id);
fpga_result opae_properties_set_device_id(fpga_properties props, uint16_t device_id);

fpga_result opae_properties_get_num_slots(fpga_properties props, uint32_t *num_slots);
fpga_result opae_properties_set_num_slots(fpga_properties props, uint32_t num_slots);

fpga_result opae_properties_get_bbs_id(fpga_properties props, uint64_t *bbs_id);
fpga_result opae_properties_set_bbs_id(fpga_properties props, uint64_t bbs_id);

fpga_result opae_properties_get_bbs_version(fpga_properties props, fpga_version *bbs_version);
fpga_result opae_properties_set_bbs_version(fpga_properties props, fpga_version bbs_version);

fpga_result opae_properties_get_vendor_id(fpga_properties props, uint16_t *vendor_id);
fpga_result opae_properties_set_vendor_id(fpga_properties props, uint16_t vendor_id);

fpga_result opae_properties_get_model(fpga_properties props, char *model);
fpga_result opae_properties_set_model(fpga_properties props, char *model);

fpga_result opae_properties_get_local_memory_size(fpga_properties props, uint64_t *lms);
fpga_result opae_properties_set_local_memory_size(fpga_properties props, uint64_t lms);

fpga_result opae_properties_get_capabilities(fpga_properties props, uint64_t *capabilities);
fpga_result opae_properties_set_capabilities(fpga_properties props, uint64_t capabilities);

fpga_result opae_properties_get_guid(fpga_properties props, fpga_guid *guid);
fpga_result opae_properties_set_guid(fpga_properties props, fpga_guid guid);

fpga_result opae_properties_get_num_mmio(fpga_properties props, uint32_t *mmio_spaces);
fpga_result opae_properties_set_num_mmio(fpga_properties props, uint32_t mmio_spaces);

fpga_result opae_properties_get_num_interrupts(fpga_properties props, uint32_t *num_interrupts);
fpga_result opae_properties_set_num_interrupts(fpga_properties props, uint32_t num_interrupts);

fpga_result opae_properties_get_accelerator_state(fpga_properties props,
						  fpga_accelerator_state *state);
fpga_result opae_properties_set_accelerator_state(fpga_properties props,
						  fpga_accelerator_state state);

fpga_result opae_properties_get_object_id(fpga_properties props, uint64_t *object_id);
fpga_result opae_properties_set_object_id(fpga_properties props, uint64_t object_id);

fpga_result opae_properties_get_num_errors(fpga_properties props, uint32_t *num_errors);
fpga_result opae_properties_set_num_errors(fpga_properties props, uint32_t num_errors);

#endif // ___OPAE_PROPS_H__

// Copyright(c) 2022, Intel Corporation
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

#include <opae/properties.h>

#include <inttypes.h>
#include <string.h>

#include "serialize.h"
#include "props.h"
#include "cfg-file.h"
#include "mock/opae_std.h"

STATIC struct json_object *
opae_ser_version_to_json(fpga_version *v)
{
	struct json_object *version;

	version = json_object_new_object();
	if (!version) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	json_object_object_add(version, "serialized_type",
			json_object_new_string("fpga_version"));
	json_object_object_add(version, "major",
			json_object_new_int(v->major));
	json_object_object_add(version, "minor",
			json_object_new_int(v->minor));
	json_object_object_add(version, "patch",
			json_object_new_int(v->patch));

	return version;
}

STATIC bool
opae_ser_json_to_version(struct json_object *jobj, fpga_version *ver)
{
	fpga_version version = { 0, 0, 0 };
	struct json_object *serialized_type;
	struct json_object *o;
	char *value;
	int n;

	serialized_type =
		parse_json_string(jobj, "serialized_type", &value);

	if (!serialized_type || strcmp(value, "fpga_version")) {
		OPAE_ERR("fpga_version de-serialize failed");
		return false;
	}

	n = 0;
	o = parse_json_int(jobj, "major", &n);
	if (!o) {
		OPAE_ERR("fpga_version de-serialize failed (major)");
		return false;
	}
	version.major = (uint8_t)n;

	n = 0;
	o = parse_json_int(jobj, "minor", &n);
	if (!o) {
		OPAE_ERR("fpga_version de-serialize failed (minor)");
		return false;
	}
	version.minor = (uint8_t)n;

	n = 0;
	o = parse_json_int(jobj, "patch", &n);
	if (!o) {
		OPAE_ERR("fpga_version de-serialize failed (patch)");
		return false;
	}
	version.patch = (uint16_t)n;

	*ver = version;
	return true;
}

#define NUM_FPGA_OBJTYPES 2
STATIC struct {
	fpga_objtype objtype;
	const char *str;
} objtype_table[NUM_FPGA_OBJTYPES] = {
	{ FPGA_DEVICE,      "FPGA_DEVICE"      },
	{ FPGA_ACCELERATOR, "FPGA_ACCELERATOR" }
};

STATIC const char *
opae_objtype_to_str(fpga_objtype t)
{
	int i;

	for (i = 0 ; i < NUM_FPGA_OBJTYPES ; ++i) {
		if (t == objtype_table[i].objtype)
			return objtype_table[i].str;
	}

	return "<unknown>";
}

STATIC fpga_objtype
opae_str_to_objtype(const char *s)
{
	int i;

	for (i = 0 ; i < NUM_FPGA_OBJTYPES ; ++i) {
		if (!strcmp(s, objtype_table[i].str))
			return objtype_table[i].objtype;
	}

	return (fpga_objtype)-1;
}

#define NUM_FPGA_IFCS 3
STATIC struct {
	fpga_interface ifc;
	const char *str;
} ifc_table[NUM_FPGA_IFCS] = {
	{ FPGA_IFC_DFL,  "FPGA_IFC_DFL"  },
	{ FPGA_IFC_VFIO, "FPGA_IFC_VFIO" },
	{ FPGA_IFC_SIM,  "FPGA_IFC_SIM"  }
};

STATIC const char *
opae_interface_to_str(fpga_interface ifc)
{
	int i;

	for (i = 0 ; i < NUM_FPGA_IFCS ; ++i) {
		if (ifc == ifc_table[i].ifc)
			return ifc_table[i].str;
	}

	return "<unknown>";
}

STATIC fpga_interface
opae_str_to_interface(const char *s)
{
	int i;

	for (i = 0 ; i < NUM_FPGA_IFCS ; ++i) {
		if (!strcmp(s, ifc_table[i].str))
			return ifc_table[i].ifc;
	}

	return (fpga_interface)-1;
}

#define NUM_ACC_STATES 2
STATIC struct {
	fpga_accelerator_state state;
	const char *str;
} acc_state_table[NUM_ACC_STATES] = {
	{ FPGA_ACCELERATOR_ASSIGNED,   "FPGA_ACCELERATOR_ASSIGNED"   },
	{ FPGA_ACCELERATOR_UNASSIGNED, "FPGA_ACCELERATOR_UNASSIGNED" }
};

STATIC const char *
opae_accelerator_state_to_str(fpga_accelerator_state st)
{
	int i;

	for (i = 0 ; i < NUM_ACC_STATES ; ++i) {
		if (st == acc_state_table[i].state)
			return acc_state_table[i].str;
	}

	return "<unknown>";
}

STATIC fpga_accelerator_state
opae_str_to_accelerator_state(const char *s)
{
	int i;

	for (i = 0 ; i < NUM_ACC_STATES ; ++i) {
		if (!strcmp(s, acc_state_table[i].str))
			return acc_state_table[i].state;
	}

	return (fpga_accelerator_state)-1;
}

bool opae_ser_properties_to_json_obj(fpga_properties prop,
                                     struct json_object *parent)
{
	struct _fpga_properties *p;
	int res;

	p = opae_validate_and_lock_properties(prop);
	if (!p) {
		OPAE_ERR("invalid properties");
		return false;
	}

	json_object_object_add(parent, "serialized_type",
			json_object_new_string("fpga_properties"));

	if (FIELD_VALID(p, FPGA_PROPERTY_PARENT)) {

	}

	if (FIELD_VALID(p, FPGA_PROPERTY_OBJTYPE)) {
		const char *s = opae_objtype_to_str(p->objtype);
		json_object_object_add(parent,
				       "objtype",
				       json_object_new_string(s));
	}

	if (FIELD_VALID(p, FPGA_PROPERTY_SEGMENT)) {
		char buf[32];
		if (snprintf(buf, sizeof(buf),
			     "0x%04" PRIx16, p->segment) >= (int)sizeof(buf)) {
			OPAE_ERR("snprintf() buffer overflow");
		}
		json_object_object_add(parent,
				       "segment",
				       json_object_new_string(buf));
	}

	if (FIELD_VALID(p, FPGA_PROPERTY_BUS)) {
		char buf[16];
		if (snprintf(buf, sizeof(buf),
			     "0x%02" PRIx8, p->bus) >= (int)sizeof(buf)) {
			OPAE_ERR("snprintf() buffer overflow");
		}
		json_object_object_add(parent,
				       "bus",
				       json_object_new_string(buf));
	}

	if (FIELD_VALID(p, FPGA_PROPERTY_DEVICE)) {
		char buf[16];
		if (snprintf(buf, sizeof(buf),
			     "0x%02" PRIx8, p->device) >= (int)sizeof(buf)) {
			OPAE_ERR("snprintf() buffer overflow");
		}
		json_object_object_add(parent,
				       "device",
				       json_object_new_string(buf));
	}

	if (FIELD_VALID(p, FPGA_PROPERTY_FUNCTION)) {
		char buf[16];
		if (snprintf(buf, sizeof(buf),
			     "%d", p->function) >= (int)sizeof(buf)) {
			OPAE_ERR("snprintf() buffer overflow");
		}
		json_object_object_add(parent,
				       "function",
				       json_object_new_string(buf));
	}

	if (FIELD_VALID(p, FPGA_PROPERTY_SOCKETID)) {
		json_object_object_add(parent,
				       "socket_id",
				       json_object_new_int(p->socket_id));
	}

	if (FIELD_VALID(p, FPGA_PROPERTY_VENDORID)) {
		char buf[32];
		if (snprintf(buf, sizeof(buf),
			     "0x%04" PRIx16, p->vendor_id) >=
				(int)sizeof(buf)) {
			OPAE_ERR("snprintf() buffer overflow");
		}
		json_object_object_add(parent,
				       "vendor_id",
				       json_object_new_string(buf));
	}

	if (FIELD_VALID(p, FPGA_PROPERTY_DEVICEID)) {
		char buf[32];
		if (snprintf(buf, sizeof(buf),
			     "0x%04" PRIx16, p->device_id) >=
				(int)sizeof(buf)) {
			OPAE_ERR("snprintf() buffer overflow");
		}
		json_object_object_add(parent,
				       "device_id",
				       json_object_new_string(buf));
	}

	if (FIELD_VALID(p, FPGA_PROPERTY_GUID)) {
		char buf[64];
		uuid_unparse(p->guid, buf);
		json_object_object_add(parent,
				       "guid",
				       json_object_new_string(buf));
	}

	if (FIELD_VALID(p, FPGA_PROPERTY_OBJECTID)) {
		char buf[32];
		if (snprintf(buf, sizeof(buf),
			     "0x%016" PRIx64, p->object_id) >=
				(int)sizeof(buf)) {
			OPAE_ERR("snprintf() buffer overflow");
		}
		json_object_object_add(parent,
				       "object_id",
				       json_object_new_string(buf));
	}

	if (FIELD_VALID(p, FPGA_PROPERTY_NUM_ERRORS)) {
		json_object_object_add(parent,
				       "num_errors",
				       json_object_new_int(p->num_errors));
	}

	if (FIELD_VALID(p, FPGA_PROPERTY_INTERFACE)) {
		const char *s = opae_interface_to_str(p->interface);
		json_object_object_add(parent,
				       "interface",
				       json_object_new_string(s));
	}

	if (FIELD_VALID(p, FPGA_PROPERTY_SUB_VENDORID)) {
		char buf[32];
		if (snprintf(buf, sizeof(buf),
			     "0x%04" PRIx16, p->subsystem_vendor_id) >=
				(int)sizeof(buf)) {
			OPAE_ERR("snprintf() buffer overflow");
		}
		json_object_object_add(parent,
				       "subsystem_vendor_id",
				       json_object_new_string(buf));
	}

	if (FIELD_VALID(p, FPGA_PROPERTY_SUB_DEVICEID)) {
		char buf[32];
		if (snprintf(buf, sizeof(buf),
			     "0x%04" PRIx16, p->subsystem_device_id) >=
				(int)sizeof(buf)) {
			OPAE_ERR("snprintf() buffer overflow");
		}
		json_object_object_add(parent,
				       "subsystem_device_id",
				       json_object_new_string(buf));
	}

	if (FIELD_VALID(p, FPGA_PROPERTY_HOSTNAME)) {
		json_object_object_add(parent,
				       "hostname",
				       json_object_new_string(p->hostname));
	}

        if (FIELD_VALID(p, FPGA_PROPERTY_OBJTYPE)
            && (FPGA_DEVICE == p->objtype)) {
		struct json_object *fpga_device = NULL;

		if (FIELD_VALID(p, FPGA_PROPERTY_NUM_SLOTS) ||
		    FIELD_VALID(p, FPGA_PROPERTY_BBSID) ||
		    FIELD_VALID(p, FPGA_PROPERTY_BBSVERSION)) {
			fpga_device =
				json_object_new_object();
			json_object_object_add(parent,
					       "fpga_device",
					       fpga_device);
		}

		if (FIELD_VALID(p, FPGA_PROPERTY_NUM_SLOTS)) {
			json_object_object_add(fpga_device,
					       "num_slots",
					       json_object_new_int(
						       p->u.fpga.num_slots));
		}

		if (FIELD_VALID(p, FPGA_PROPERTY_BBSID)) {
			char buf[32];
			if (snprintf(buf, sizeof(buf),
				     "0x%016" PRIx64, p->u.fpga.bbs_id) >=
					(int)sizeof(buf)) {
				OPAE_ERR("snprintf() buffer overflow");
			}
			json_object_object_add(fpga_device,
					       "bbs_id",
					       json_object_new_string(buf));
		}

		if (FIELD_VALID(p, FPGA_PROPERTY_BBSVERSION)) {
			json_object *version =
				opae_ser_version_to_json(
						&p->u.fpga.bbs_version);
			json_object_object_add(fpga_device,
					       "bbs_version",
					       version);
		}

	} else if (FIELD_VALID(p, FPGA_PROPERTY_OBJTYPE)
                   && (FPGA_ACCELERATOR == p->objtype)) {
		struct json_object *fpga_accelerator = NULL;

		if (FIELD_VALID(p, FPGA_PROPERTY_ACCELERATOR_STATE) ||
		    FIELD_VALID(p, FPGA_PROPERTY_NUM_MMIO) ||
		    FIELD_VALID(p, FPGA_PROPERTY_NUM_INTERRUPTS)) {
			fpga_accelerator =
				json_object_new_object();
			json_object_object_add(parent,
					       "fpga_accelerator",
					       fpga_accelerator);
		}

		if (FIELD_VALID(p, FPGA_PROPERTY_ACCELERATOR_STATE)) {
			const char *s = opae_accelerator_state_to_str(
					p->u.accelerator.state);
			json_object_object_add(fpga_accelerator,
					       "state",
					       json_object_new_string(s));
		}

		if (FIELD_VALID(p, FPGA_PROPERTY_NUM_MMIO)) {
			json_object_object_add(fpga_accelerator,
					       "num_mmio",
				json_object_new_int(
					p->u.accelerator.num_mmio));
		}

		if (FIELD_VALID(p, FPGA_PROPERTY_NUM_INTERRUPTS)) {
			json_object_object_add(fpga_accelerator,
					       "num_interrupts",
				json_object_new_int(
					p->u.accelerator.num_interrupts));
		}

	}

	opae_mutex_unlock(res, &p->lock);
	return true;
}

char *opae_ser_properties_to_json(fpga_properties prop, int json_flags)
{
	struct json_object *root;
	char *json = NULL;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_ser_properties_to_json_obj(prop, root))
		goto out_err;

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_err:
	json_object_put(root);
	return json;
}

bool opae_ser_json_to_properties_obj(struct json_object *jobj,
                                     fpga_properties *props)
{
	fpga_result res;
	struct _fpga_properties *p;
	char *str;
	int n;
	json_object *fpga_device = NULL;
	json_object *fpga_accelerator = NULL;
	json_object *obj;

	res = fpgaGetProperties(NULL, props);
	if (res)
		return false;

	p = (struct _fpga_properties *)(*props);

	// FPGA_PROPERTY_PARENT

	str = NULL;
	obj = parse_json_string(jobj, "serialized_type", &str);
	if (!obj || strcmp(str, "fpga_properties")) {
		OPAE_ERR("fpga_properties de-serialize failed");
		goto out_destroy;
	}

	str = NULL;
	obj = parse_json_string(jobj, "objtype", &str);
	if (obj) {
		fpga_objtype objtype = opae_str_to_objtype(str);
		SET_FIELD_VALID(p, FPGA_PROPERTY_OBJTYPE);
		p->objtype = objtype;
	}

	str = NULL;
	obj = parse_json_string(jobj, "segment", &str);
	if (obj) {
		uint16_t segment;
		char *endptr = NULL;

		segment = (uint16_t)strtoul(str, &endptr, 0);
		if (endptr != str + strlen(str)) {
			OPAE_ERR("fpga_properties de-serialize "
				 "failed (segment)");
			goto out_destroy;
		}

		SET_FIELD_VALID(p, FPGA_PROPERTY_SEGMENT);
		p->segment = segment;
	}

	str = NULL;
	obj = parse_json_string(jobj, "bus", &str);
	if (obj) {
		uint8_t bus;
		char *endptr = NULL;

		bus = (uint8_t)strtoul(str, &endptr, 0);
		if (endptr != str + strlen(str)) {
			OPAE_ERR("fpga_properties de-serialize "
				 "failed (bus)");
			goto out_destroy;
		}

		SET_FIELD_VALID(p, FPGA_PROPERTY_BUS);
		p->bus = bus;
	}

	str = NULL;
	obj = parse_json_string(jobj, "device", &str);
	if (obj) {
		uint8_t device;
		char *endptr = NULL;

		device = (uint8_t)strtoul(str, &endptr, 0);
		if (endptr != str + strlen(str)) {
			OPAE_ERR("fpga_properties de-serialize "
				 "failed (device)");
			goto out_destroy;
		}

		SET_FIELD_VALID(p, FPGA_PROPERTY_DEVICE);
		p->device = device;
	}

	str = NULL;
	obj = parse_json_string(jobj, "function", &str);
	if (obj) {
		uint8_t function;
		char *endptr = NULL;

		function = (uint8_t)strtoul(str, &endptr, 0);
		if (endptr != str + strlen(str)) {
			OPAE_ERR("fpga_properties de-serialize "
				 "failed (function)");
			goto out_destroy;
		}

		SET_FIELD_VALID(p, FPGA_PROPERTY_FUNCTION);
		p->function = function;
	}

	n = 0;
	obj = parse_json_int(jobj, "socket_id", &n);
	if (obj) {
		SET_FIELD_VALID(p, FPGA_PROPERTY_SOCKETID);
		p->socket_id = n;
	}

	str = NULL;
	obj = parse_json_string(jobj, "vendor_id", &str);
	if (obj) {
		uint16_t vendor_id;
		char *endptr = NULL;

		vendor_id = (uint16_t)strtoul(str, &endptr, 0);
		if (endptr != str + strlen(str)) {
			OPAE_ERR("fpga_properties de-serialize "
				 "failed (vendor_id)");
			goto out_destroy;
		}

		SET_FIELD_VALID(p, FPGA_PROPERTY_VENDORID);
		p->vendor_id = vendor_id;
	}

	str = NULL;
	obj = parse_json_string(jobj, "device_id", &str);
	if (obj) {
		uint16_t device_id;
		char *endptr = NULL;

		device_id = (uint16_t)strtoul(str, &endptr, 0);
		if (endptr != str + strlen(str)) {
			OPAE_ERR("fpga_properties de-serialize "
				 "failed (device_id)");
			goto out_destroy;
		}

		SET_FIELD_VALID(p, FPGA_PROPERTY_DEVICEID);
		p->device_id = device_id;
	}

	str = NULL;
	obj = parse_json_string(jobj, "guid", &str);
	if (obj) {
		SET_FIELD_VALID(p, FPGA_PROPERTY_GUID);
		uuid_parse(str, p->guid);
	}

	str = NULL;
	obj = parse_json_string(jobj, "object_id", &str);
	if (obj) {
		uint64_t object_id;
		char *endptr = NULL;

		object_id = strtoul(str, &endptr, 0);
		if (endptr != str + strlen(str)) {
			OPAE_ERR("fpga_properties de-serialize "
				 "failed (object_id)");
			goto out_destroy;
		}

		SET_FIELD_VALID(p, FPGA_PROPERTY_OBJECTID);
		p->object_id = object_id;
	}

	n = 0;
	obj = parse_json_int(jobj, "num_errors", &n);
	if (obj) {
		SET_FIELD_VALID(p, FPGA_PROPERTY_NUM_ERRORS);
		p->num_errors = n;
	}

	str = NULL;
	obj = parse_json_string(jobj, "interface", &str);
	if (obj) {
		fpga_interface ifc = opae_str_to_interface(str);
		SET_FIELD_VALID(p, FPGA_PROPERTY_INTERFACE);
		p->interface = ifc;
	}

	str = NULL;
	obj = parse_json_string(jobj, "subsystem_vendor_id", &str);
	if (obj) {
		uint16_t subsystem_vendor_id;
		char *endptr = NULL;

		subsystem_vendor_id = (uint16_t)strtoul(str, &endptr, 0);
		if (endptr != str + strlen(str)) {
			OPAE_ERR("fpga_properties de-serialize "
				 "failed (subsystem_vendor_id)");
			goto out_destroy;
		}

		SET_FIELD_VALID(p, FPGA_PROPERTY_SUB_VENDORID);
		p->subsystem_vendor_id = subsystem_vendor_id;
	}

	str = NULL;
	obj = parse_json_string(jobj, "subsystem_device_id", &str);
	if (obj) {
		uint16_t subsystem_device_id;
		char *endptr = NULL;

		subsystem_device_id = (uint16_t)strtoul(str, &endptr, 0);
		if (endptr != str + strlen(str)) {
			OPAE_ERR("fpga_properties de-serialize "
				 "failed (subsystem_device_id)");
			goto out_destroy;
		}

		SET_FIELD_VALID(p, FPGA_PROPERTY_SUB_DEVICEID);
		p->subsystem_device_id = subsystem_device_id;
	}

	str = NULL;
	obj = parse_json_string(jobj, "hostname", &str);
	if (obj) {
		size_t len = strlen(str);

		if (len > HOST_NAME_MAX)
			len = HOST_NAME_MAX;
		memcpy(p->hostname, str, len);
		p->hostname[len] = '\0';

		SET_FIELD_VALID(p, FPGA_PROPERTY_HOSTNAME);
	}

	if (json_object_object_get_ex(jobj, "fpga_device", &fpga_device)) {
		struct json_object *bbs_version = NULL;

		n = 0;
		obj = parse_json_int(fpga_device, "num_slots", &n);
		if (obj) {
			SET_FIELD_VALID(p, FPGA_PROPERTY_NUM_SLOTS);
			p->u.fpga.num_slots = n;
		}

		str = NULL;
		obj = parse_json_string(fpga_device, "bbs_id", &str);
		if (obj) {
			uint64_t bbs_id;
			char *endptr = NULL;

			bbs_id = strtoul(str, &endptr, 0);
			if (endptr != str + strlen(str)) {
				OPAE_ERR("fpga_properties de-serialize "
					 "failed (bbs_id)");
				goto out_destroy;
			}

			SET_FIELD_VALID(p, FPGA_PROPERTY_BBSID);
			p->u.fpga.bbs_id = bbs_id;
		}

		if (json_object_object_get_ex(fpga_device,
					      "bbs_version",
					      &bbs_version)) {
			if (!opae_ser_json_to_version(bbs_version,
						&p->u.fpga.bbs_version)) {
				OPAE_ERR("fpga_properties de-serialize "
					 "failed (bbs_version)");
				goto out_destroy;
			}
			SET_FIELD_VALID(p, FPGA_PROPERTY_BBSVERSION);
		}

	}

	if (json_object_object_get_ex(jobj, "fpga_accelerator",
				&fpga_accelerator)) {

		str = NULL;
		obj = parse_json_string(fpga_accelerator, "state", &str);
		if (obj) {
			fpga_accelerator_state state =
				opae_str_to_accelerator_state(str);
			SET_FIELD_VALID(p, FPGA_PROPERTY_ACCELERATOR_STATE);
			p->u.accelerator.state = state;
		}

		n = 0;
		obj = parse_json_int(fpga_accelerator, "num_mmio", &n);
		if (obj) {
			SET_FIELD_VALID(p, FPGA_PROPERTY_NUM_MMIO);
			p->u.accelerator.num_mmio = n;
		}

		n = 0;
		obj = parse_json_int(fpga_accelerator, "num_interrupts", &n);
		if (obj) {
			SET_FIELD_VALID(p, FPGA_PROPERTY_NUM_INTERRUPTS);
			p->u.accelerator.num_interrupts = n;
		}

	}

	return true;

out_destroy:
	fpgaDestroyProperties(props);
	return false;
}

bool opae_ser_json_to_properties(const char *json,
				 fpga_properties *props)
{
	json_object *root = NULL;
	enum json_tokener_error j_err = json_tokener_success;
	bool res = false;

	root = json_tokener_parse_verbose(json, &j_err);
	if (!root) {
		OPAE_ERR("JSON parse failed: %s",
                         json_tokener_error_desc(j_err));
                return false;
	}

	if (!opae_ser_json_to_properties_obj(root, props))
		goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}

bool opae_ser_token_header_to_json_obj(fpga_token_header *hdr,
                                       struct json_object *parent)
{
	char buf[64];
	const char *str;

	json_object_object_add(parent, "serialized_type",
			json_object_new_string("fpga_token_header"));

	if (snprintf(buf, sizeof(buf),
		     "0x%016" PRIx64, hdr->magic) >=
			(int)sizeof(buf)) {
		OPAE_ERR("snprintf() buffer overflow");
	}
	json_object_object_add(parent,
			       "magic",
			       json_object_new_string(buf));

	if (snprintf(buf, sizeof(buf),
		     "0x%04" PRIx16, hdr->vendor_id) >=
			(int)sizeof(buf)) {
		OPAE_ERR("snprintf() buffer overflow");
	}
	json_object_object_add(parent,
			       "vendor_id",
			       json_object_new_string(buf));

	if (snprintf(buf, sizeof(buf),
		     "0x%04" PRIx16, hdr->device_id) >=
			(int)sizeof(buf)) {
		OPAE_ERR("snprintf() buffer overflow");
	}
	json_object_object_add(parent,
			       "device_id",
			       json_object_new_string(buf));

	if (snprintf(buf, sizeof(buf),
		     "0x%04" PRIx16, hdr->segment) >=
			(int)sizeof(buf)) {
		OPAE_ERR("snprintf() buffer overflow");
	}
	json_object_object_add(parent,
			       "segment",
			       json_object_new_string(buf));

	if (snprintf(buf, sizeof(buf),
		     "0x%02" PRIx8, hdr->bus) >= (int)sizeof(buf)) {
		OPAE_ERR("snprintf() buffer overflow");
	}
	json_object_object_add(parent,
			       "bus",
			       json_object_new_string(buf));

	if (snprintf(buf, sizeof(buf),
		     "0x%02" PRIx8, hdr->device) >= (int)sizeof(buf)) {
		OPAE_ERR("snprintf() buffer overflow");
	}
	json_object_object_add(parent,
			       "device",
			       json_object_new_string(buf));

	if (snprintf(buf, sizeof(buf),
		     "%d", hdr->function) >= (int)sizeof(buf)) {
		OPAE_ERR("snprintf() buffer overflow");
	}
	json_object_object_add(parent,
			       "function",
			       json_object_new_string(buf));

	str = opae_interface_to_str(hdr->interface);
	json_object_object_add(parent,
			       "interface",
			       json_object_new_string(str));

	str = opae_objtype_to_str(hdr->objtype);
	json_object_object_add(parent,
			       "objtype",
			       json_object_new_string(str));

	if (snprintf(buf, sizeof(buf),
		     "0x%016" PRIx64, hdr->object_id) >=
			(int)sizeof(buf)) {
		OPAE_ERR("snprintf() buffer overflow");
	}
	json_object_object_add(parent,
			       "object_id",
			       json_object_new_string(buf));

	uuid_unparse(hdr->guid, buf);
	json_object_object_add(parent,
			       "guid",
			       json_object_new_string(buf));

	if (snprintf(buf, sizeof(buf),
		     "0x%04" PRIx16, hdr->subsystem_vendor_id) >=
			(int)sizeof(buf)) {
		OPAE_ERR("snprintf() buffer overflow");
	}
	json_object_object_add(parent,
			       "subsystem_vendor_id",
			       json_object_new_string(buf));

	if (snprintf(buf, sizeof(buf),
		     "0x%04" PRIx16, hdr->subsystem_device_id) >=
			(int)sizeof(buf)) {
		OPAE_ERR("snprintf() buffer overflow");
	}
	json_object_object_add(parent,
			       "subsystem_device_id",
			       json_object_new_string(buf));

	json_object_object_add(parent,
			       "hostname",
			       json_object_new_string(hdr->hostname));

	json_object_object_add(parent,
			       "remote_id",
			       json_object_new_int(hdr->remote_id));

	return true;
}

char *opae_ser_token_header_to_json(fpga_token_header *hdr,
                                    int json_flags)
{
	struct json_object *root = NULL;
	char *json = NULL;

	root = json_object_new_object();
	if (!root) {
		OPAE_ERR("out of memory");
		return NULL;
	}

	if (!opae_ser_token_header_to_json_obj(hdr, root))
		goto out_put;

	json = opae_strdup(json_object_to_json_string_ext(root, json_flags));

out_put:
	json_object_put(root);
	return json;
}

bool opae_ser_json_to_token_header_obj(struct json_object *jobj,
                                       fpga_token_header *hdr)
{
	char *str;
	char *endptr;
	uint8_t u8;
	uint16_t u16;
	uint64_t u64;
	size_t len;

	str = endptr = NULL;
	if (!parse_json_string(jobj, "magic", &str))
		return false;

	u64 = strtoul(str, &endptr, 0);
	if (endptr != str + strlen(str)) {
		OPAE_ERR("fpga_token_header de-serialize "
			 "failed (magic)");
		return false;
	}
	hdr->magic = u64;

	str = endptr = NULL;
	if (!parse_json_string(jobj, "vendor_id", &str))
		return false;

	u16 = (uint16_t)strtoul(str, &endptr, 0);
	if (endptr != str + strlen(str)) {
		OPAE_ERR("fpga_token_header de-serialize "
			 "failed (vendor_id)");
		return false;
	}
	hdr->vendor_id = u16;

	str = endptr = NULL;
	if (!parse_json_string(jobj, "device_id", &str))
		return false;

	u16 = (uint16_t)strtoul(str, &endptr, 0);
	if (endptr != str + strlen(str)) {
		OPAE_ERR("fpga_token_header de-serialize "
			 "failed (device_id)");
		return false;
	}
	hdr->device_id = u16;

	str = endptr = NULL;
	if (!parse_json_string(jobj, "segment", &str))
		return false;

	u16 = (uint16_t)strtoul(str, &endptr, 0);
	if (endptr != str + strlen(str)) {
		OPAE_ERR("fpga_token_header de-serialize "
			 "failed (segment)");
		return false;
	}
	hdr->segment = u16;

	str = endptr = NULL;
	if (!parse_json_string(jobj, "bus", &str))
		return false;

	u8 = (uint8_t)strtoul(str, &endptr, 0);
	if (endptr != str + strlen(str)) {
		OPAE_ERR("fpga_token_header de-serialize "
			 "failed (bus)");
		return false;
	}
	hdr->bus = u8;

	str = endptr = NULL;
	if (!parse_json_string(jobj, "device", &str))
		return false;

	u8 = (uint8_t)strtoul(str, &endptr, 0);
	if (endptr != str + strlen(str)) {
		OPAE_ERR("fpga_token_header de-serialize "
			 "failed (device)");
		return false;
	}
	hdr->device = u8;

	str = endptr = NULL;
	if (!parse_json_string(jobj, "function", &str))
		return false;

	u8 = (uint8_t)strtoul(str, &endptr, 0);
	if (endptr != str + strlen(str)) {
		OPAE_ERR("fpga_token_header de-serialize "
			 "failed (function)");
		return false;
	}
	hdr->function = u8;

	str = NULL;
	if (!parse_json_string(jobj, "interface", &str))
		return false;

	hdr->interface = opae_str_to_interface(str);

	str = NULL;
	if (!parse_json_string(jobj, "objtype", &str))
		return false;

	hdr->objtype = opae_str_to_objtype(str);

	str = endptr = NULL;
	if (!parse_json_string(jobj, "object_id", &str))
		return false;

	u64 = strtoul(str, &endptr, 0);
	if (endptr != str + strlen(str)) {
		OPAE_ERR("fpga_token_header de-serialize "
			 "failed (object_id)");
		return false;
	}
	hdr->object_id = u64;

	str = NULL;
	if (!parse_json_string(jobj, "guid", &str))
		return false;

	uuid_parse(str, hdr->guid);

	str = endptr = NULL;
	if (!parse_json_string(jobj, "subsystem_vendor_id", &str))
		return false;

	u16 = (uint16_t)strtoul(str, &endptr, 0);
	if (endptr != str + strlen(str)) {
		OPAE_ERR("fpga_token_header de-serialize "
			 "failed (subsystem_vendor_id)");
		return false;
	}
	hdr->subsystem_vendor_id = u16;

	str = endptr = NULL;
	if (!parse_json_string(jobj, "subsystem_device_id", &str))
		return false;

	u16 = (uint16_t)strtoul(str, &endptr, 0);
	if (endptr != str + strlen(str)) {
		OPAE_ERR("fpga_token_header de-serialize "
			 "failed (subsystem_device_id)");
		return false;
	}
	hdr->subsystem_device_id = u16;

	str = NULL;
	if (!parse_json_string(jobj, "hostname", &str))
		return false;

	len = strlen(str);
	if (len > HOST_NAME_MAX)
		len = HOST_NAME_MAX;
	memcpy(hdr->hostname, str, len);
	hdr->hostname[len] = '\0';

	if (!parse_json_u64(jobj, "remote_id", &hdr->remote_id))
		return false;

	return true;
}

bool opae_ser_json_to_token_header(const char *json,
				   fpga_token_header *hdr)
{
	json_object *root = NULL;
	enum json_tokener_error j_err = json_tokener_success;
	bool res = false;

	root = json_tokener_parse_verbose(json, &j_err);
	if (!root) {
		OPAE_ERR("JSON parse failed: %s",
                         json_tokener_error_desc(j_err));
                return false;
	}

	if (!opae_ser_json_to_token_header_obj(root, hdr))
		goto out_put;

	res = true;

out_put:
	json_object_put(root);
	return res;
}

#define NUM_FPGA_RESULTS 11
STATIC struct {
	fpga_result res;
	const char *str;
} result_table[NUM_FPGA_RESULTS] = {
	{ FPGA_OK,            "FPGA_OK"            },
	{ FPGA_INVALID_PARAM, "FPGA_INVALID_PARAM" },
	{ FPGA_BUSY,          "FPGA_BUSY"          },
	{ FPGA_EXCEPTION,     "FPGA_EXCEPTION"     },
	{ FPGA_NOT_FOUND,     "FPGA_NOT_FOUND"     },
	{ FPGA_NO_MEMORY,     "FPGA_NO_MEMORY"     },
	{ FPGA_NOT_SUPPORTED, "FPGA_NOT_SUPPORTED" },
	{ FPGA_NO_DRIVER,     "FPGA_NO_DRIVER"     },
	{ FPGA_NO_DAEMON,     "FPGA_NO_DAEMON"     },
	{ FPGA_NO_ACCESS,     "FPGA_NO_ACCESS"     },
	{ FPGA_RECONF_ERROR,  "FPGA_RECONF_ERROR"  }
};

STATIC const char *
opae_fpga_result_to_str(fpga_result res)
{
	int i;

	for (i = 0 ; i < NUM_FPGA_RESULTS ; ++i) {
		if (res == result_table[i].res)
			return result_table[i].str;
	}

	return "<unknown>";
}

STATIC fpga_result
opae_str_to_fpga_result(const char *s)
{
	int i;

	for (i = 0 ; i < NUM_FPGA_RESULTS ; ++i) {
		if (!strcmp(s, result_table[i].str))
			return result_table[i].res;
	}

	return (fpga_result)-1;
}

bool opae_ser_fpga_result_to_json_obj(fpga_result res,
                                      struct json_object *parent)
{
	const char *str;

	str = opae_fpga_result_to_str(res);
	json_object_object_add(parent, "fpga_result",
		json_object_new_string(str));

	return true;
}

bool opae_ser_json_to_fpga_result_obj(struct json_object *jobj,
                                      fpga_result *res)
{
	char *str = NULL;
	struct json_object *obj;

	obj = parse_json_string(jobj, "fpga_result", &str);
	if (!obj)
		return false;

	*res = opae_str_to_fpga_result(str);

	return true;
}

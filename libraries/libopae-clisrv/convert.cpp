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
#endif  // HAVE_CONFIG_H

#include <google/protobuf/util/json_util.h>
#include <opae/types.h>
#include <opae/types_enum.h>
#include <uuid/uuid.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <sstream>

#include "grpc_client.hpp"
#include "grpc_server.hpp"
#include "opae_int.h"
#include "props.h"
#include "remote.h"
using namespace google::protobuf;

std::ostream &operator<<(std::ostream &os, const grpc::protobuf::Message &m) {
  util::JsonOptions opts;
  opts.add_whitespace = true;
  opts.always_print_enums_as_ints = false;
  opts.always_print_primitive_fields = true;
  opts.preserve_proto_field_names = true;

  std::string msg_str;
  util::MessageToJsonString(m, &msg_str, opts);

  return os << msg_str;
}

std::string *to_string(const fpga_guid guid) {
  char buf[64];
  uuid_unparse(guid, buf);
  return new std::string(buf);
}

void from_string(const std::string &s, fpga_guid &guid) {
  uuid_parse(s.c_str(), guid);
}

const opaegrpc::fpga_result
    to_grpc_fpga_result[opaegrpc::fpga_result_ARRAYSIZE] = {
        opaegrpc::FPGA_OK,
        opaegrpc::FPGA_INVALID_PARAM,
        opaegrpc::FPGA_BUSY,
        opaegrpc::FPGA_EXCEPTION,
        opaegrpc::FPGA_NOT_FOUND,
        opaegrpc::FPGA_NO_MEMORY,
        opaegrpc::FPGA_NOT_SUPPORTED,
        opaegrpc::FPGA_NO_DRIVER,
        opaegrpc::FPGA_NO_DAEMON,
        opaegrpc::FPGA_NO_ACCESS,
        opaegrpc::FPGA_RECONF_ERROR};

const fpga_result to_opae_fpga_result[opaegrpc::fpga_result_ARRAYSIZE] = {
    FPGA_OK,        FPGA_INVALID_PARAM, FPGA_BUSY,          FPGA_EXCEPTION,
    FPGA_NOT_FOUND, FPGA_NO_MEMORY,     FPGA_NOT_SUPPORTED, FPGA_NO_DRIVER,
    FPGA_NO_DAEMON, FPGA_NO_ACCESS,     FPGA_RECONF_ERROR};

const opaegrpc::fpga_event_type
    to_grpc_fpga_event_type[opaegrpc::fpga_event_type_ARRAYSIZE] = {
        opaegrpc::FPGA_EVENT_INTERRUPT, opaegrpc::FPGA_EVENT_ERROR,
        opaegrpc::FPGA_EVENT_POWER_THERMAL};

const fpga_event_type
    to_opae_fpga_event_type[opaegrpc::fpga_event_type_ARRAYSIZE] = {
        FPGA_EVENT_INTERRUPT, FPGA_EVENT_ERROR, FPGA_EVENT_POWER_THERMAL};

const opaegrpc::fpga_accelerator_state
    to_grpc_fpga_accelerator_state[opaegrpc::fpga_accelerator_state_ARRAYSIZE] =
        {opaegrpc::FPGA_ACCELERATOR_ASSIGNED,
         opaegrpc::FPGA_ACCELERATOR_UNASSIGNED};

const fpga_accelerator_state
    to_opae_fpga_accelerator_state[opaegrpc::fpga_accelerator_state_ARRAYSIZE] =
        {FPGA_ACCELERATOR_ASSIGNED, FPGA_ACCELERATOR_UNASSIGNED};

const opaegrpc::fpga_objtype
    to_grpc_fpga_objtype[opaegrpc::fpga_objtype_ARRAYSIZE] = {
        opaegrpc::FPGA_DEVICE, opaegrpc::FPGA_ACCELERATOR};

const fpga_objtype to_opae_fpga_objtype[opaegrpc::fpga_objtype_ARRAYSIZE] = {
    FPGA_DEVICE, FPGA_ACCELERATOR};

const opaegrpc::fpga_interface
    to_grpc_fpga_interface[opaegrpc::fpga_interface_ARRAYSIZE] = {
        opaegrpc::FPGA_IFC_DFL, opaegrpc::FPGA_IFC_VFIO,
        opaegrpc::FPGA_IFC_SIM_DFL, opaegrpc::FPGA_IFC_SIM_VFIO};

const fpga_interface
    to_opae_fpga_interface[opaegrpc::fpga_interface_ARRAYSIZE] = {
        FPGA_IFC_DFL, FPGA_IFC_VFIO, FPGA_IFC_SIM_DFL, FPGA_IFC_SIM_VFIO};

const opaegrpc::fpga_sysobject_type
    to_grpc_fpga_sysobject_type[opaegrpc::fpga_sysobject_type_ARRAYSIZE] = {
        (opaegrpc::fpga_sysobject_type)0, opaegrpc::FPGA_OBJECT_CONTAINER,
        opaegrpc::FPGA_OBJECT_ATTRIBUTE};

const fpga_sysobject_type
    to_opae_fpga_sysobject_type[opaegrpc::fpga_sysobject_type_ARRAYSIZE] = {
        (fpga_sysobject_type)0, FPGA_OBJECT_CONTAINER, FPGA_OBJECT_ATTRIBUTE};

const opaegrpc::fpga_metric_type
    to_grpc_fpga_metric_type[opaegrpc::fpga_metric_type_ARRAYSIZE] = {
        opaegrpc::FPGA_METRIC_TYPE_POWER, opaegrpc::FPGA_METRIC_TYPE_THERMAL,
        opaegrpc::FPGA_METRIC_TYPE_PERFORMANCE_CTR,
        opaegrpc::FPGA_METRIC_TYPE_AFU, opaegrpc::FPGA_METRIC_TYPE_UNKNOWN};

const fpga_metric_type
    to_opae_fpga_metric_type[opaegrpc::fpga_metric_type_ARRAYSIZE] = {
        FPGA_METRIC_TYPE_POWER, FPGA_METRIC_TYPE_THERMAL,
        FPGA_METRIC_TYPE_PERFORMANCE_CTR, FPGA_METRIC_TYPE_AFU,
        FPGA_METRIC_TYPE_UNKNOWN};

const opaegrpc::fpga_metric_datatype
    to_grpc_fpga_metric_datatype[opaegrpc::fpga_metric_datatype_ARRAYSIZE] = {
        opaegrpc::FPGA_METRIC_DATATYPE_INT,
        opaegrpc::FPGA_METRIC_DATATYPE_FLOAT,
        opaegrpc::FPGA_METRIC_DATATYPE_DOUBLE,
        opaegrpc::FPGA_METRIC_DATATYPE_BOOL,
        opaegrpc::FPGA_METRIC_DATATYPE_UNKNOWN};

const fpga_metric_datatype
    to_opae_fpga_metric_datatype[opaegrpc::fpga_metric_datatype_ARRAYSIZE] = {
        FPGA_METRIC_DATATYPE_INT, FPGA_METRIC_DATATYPE_FLOAT,
        FPGA_METRIC_DATATYPE_DOUBLE, FPGA_METRIC_DATATYPE_BOOL,
        FPGA_METRIC_DATATYPE_UNKNOWN};

opaegrpc::fpga_version *to_grpc_fpga_version(const fpga_version &ver) {
  opaegrpc::fpga_version *gver = new opaegrpc::fpga_version();

  gver->set_major(ver.major);
  gver->set_minor(ver.minor);
  gver->set_patch(ver.patch);

  return gver;
}

fpga_version to_opae_fpga_version(const opaegrpc::fpga_version &gver) {
  fpga_version v;

  v.major = (uint8_t)gver.major();
  v.minor = (uint8_t)gver.minor();
  v.patch = (uint16_t)gver.patch();

  return v;
}

opaegrpc::fpga_error_info *to_grpc_fpga_error_info(
    const fpga_error_info &info) {
  opaegrpc::fpga_error_info *ginfo = new opaegrpc::fpga_error_info();

  ginfo->set_allocated_name(new std::string(info.name));
  ginfo->set_can_clear(info.can_clear);

  return ginfo;
}

fpga_error_info to_opae_fpga_error_info(
    const opaegrpc::fpga_error_info &ginfo) {
  fpga_error_info info;
  size_t len =
      std::min(ginfo.name().length(), (size_t)(FPGA_ERROR_NAME_MAX - 1));

  std::copy(ginfo.name().cbegin(), ginfo.name().cbegin() + len, info.name);
  info.name[len] = '\0';

  info.can_clear = ginfo.can_clear();

  return info;
}

opaegrpc::metric_value *to_grpc_metric_value(const metric_value &val) {
  opaegrpc::metric_value *gval = new opaegrpc::metric_value();
  gval->set_dvalue(val.dvalue);
  return gval;
}

metric_value to_opae_metric_value(const opaegrpc::metric_value &gval) {
  metric_value val;
  val.dvalue = gval.dvalue();
  return val;
}

void to_grpc_fpga_metric_info(const fpga_metric_info &minfo,
                              opaegrpc::fpga_metric_info *gminfo) {
  gminfo->set_metric_num(minfo.metric_num);
  gminfo->set_allocated_metric_guid(to_string(minfo.metric_guid));
  gminfo->set_allocated_qualifier_name(new std::string(minfo.qualifier_name));
  gminfo->set_allocated_group_name(new std::string(minfo.group_name));
  gminfo->set_allocated_metric_name(new std::string(minfo.metric_name));
  gminfo->set_allocated_metric_units(new std::string(minfo.metric_units));
  gminfo->set_metric_datatype(
      to_grpc_fpga_metric_datatype[minfo.metric_datatype]);
  gminfo->set_metric_type(to_grpc_fpga_metric_type[minfo.metric_type]);
}

fpga_metric_info to_opae_fpga_metric_info(
    const opaegrpc::fpga_metric_info &ginfo) {
  fpga_metric_info info;
  const size_t maxlen = (size_t)(FPGA_METRIC_STR_SIZE - 1);
  size_t len;

  info.metric_num = ginfo.metric_num();
  from_string(ginfo.metric_guid(), info.metric_guid);

  len = std::min(ginfo.qualifier_name().length(), maxlen);
  std::copy(ginfo.qualifier_name().cbegin(),
            ginfo.qualifier_name().cbegin() + len, info.qualifier_name);
  info.qualifier_name[len] = '\0';

  len = std::min(ginfo.group_name().length(), maxlen);
  std::copy(ginfo.group_name().cbegin(), ginfo.group_name().cbegin() + len,
            info.group_name);
  info.group_name[len] = '\0';

  len = std::min(ginfo.metric_name().length(), maxlen);
  std::copy(ginfo.metric_name().cbegin(), ginfo.metric_name().cbegin() + len,
            info.metric_name);
  info.metric_name[len] = '\0';

  len = std::min(ginfo.metric_units().length(), maxlen);
  std::copy(ginfo.metric_units().cbegin(), ginfo.metric_units().cbegin() + len,
            info.metric_units);
  info.metric_units[len] = '\0';

  info.metric_datatype = to_opae_fpga_metric_datatype[ginfo.metric_datatype()];
  info.metric_type = to_opae_fpga_metric_type[ginfo.metric_type()];

  return info;
}

void to_grpc_fpga_metric(const fpga_metric &m, opaegrpc::fpga_metric *gm) {
  gm->set_metric_num(m.metric_num);
  gm->set_allocated_value(to_grpc_metric_value(m.value));
  gm->set_isvalid(m.isvalid);
}

fpga_metric to_opae_fpga_metric(const opaegrpc::fpga_metric &gm) {
  fpga_metric m;

  m.metric_num = gm.metric_num();
  if (gm.has_value())
    m.value = to_opae_metric_value(gm.value());
  else
    OPAE_ERR("fpga_metric missing value");
  m.isvalid = gm.isvalid();

  return m;
}

opaegrpc::threshold *to_grpc_threshold(const threshold &t) {
  opaegrpc::threshold *gt = new opaegrpc::threshold();

  gt->set_allocated_threshold_name(new std::string(t.threshold_name));
  gt->set_is_valid(t.is_valid);
  gt->set_value(t.value);

  return gt;
}

threshold to_opae_threshold(const opaegrpc::threshold &gt) {
  threshold t;
  size_t len = std::min(gt.threshold_name().length(),
                        (size_t)(FPGA_METRIC_STR_SIZE - 1));

  std::copy(gt.threshold_name().cbegin(), gt.threshold_name().cbegin() + len,
            t.threshold_name);
  t.threshold_name[len] = '\0';

  t.is_valid = gt.is_valid();
  t.value = gt.value();

  return t;
}

void to_grpc_metric_threshold(const metric_threshold &mt,
                              opaegrpc::metric_threshold *gmt) {
  gmt->set_allocated_metric_name(new std::string(mt.metric_name));
  gmt->set_allocated_upper_nr_threshold(
      to_grpc_threshold(mt.upper_nr_threshold));
  gmt->set_allocated_upper_c_threshold(to_grpc_threshold(mt.upper_c_threshold));
  gmt->set_allocated_upper_nc_threshold(
      to_grpc_threshold(mt.upper_nc_threshold));
  gmt->set_allocated_lower_nr_threshold(
      to_grpc_threshold(mt.lower_nr_threshold));
  gmt->set_allocated_lower_c_threshold(to_grpc_threshold(mt.lower_c_threshold));
  gmt->set_allocated_lower_nc_threshold(
      to_grpc_threshold(mt.lower_nc_threshold));
  gmt->set_allocated_hysteresis(to_grpc_threshold(mt.hysteresis));
}

metric_threshold to_opae_metric_threshold(
    const opaegrpc::metric_threshold &gmt) {
  metric_threshold mt;
  const size_t len =
      std::min(gmt.metric_name().length(), (size_t)(FPGA_METRIC_STR_SIZE - 1));

  std::copy(gmt.metric_name().cbegin(), gmt.metric_name().cbegin() + len,
            mt.metric_name);
  mt.metric_name[len] = '\0';

  mt.upper_nr_threshold = to_opae_threshold(gmt.upper_nr_threshold());
  mt.upper_c_threshold = to_opae_threshold(gmt.upper_c_threshold());
  mt.upper_nc_threshold = to_opae_threshold(gmt.upper_nc_threshold());
  mt.lower_nr_threshold = to_opae_threshold(gmt.lower_nr_threshold());
  mt.lower_c_threshold = to_opae_threshold(gmt.lower_c_threshold());
  mt.lower_nc_threshold = to_opae_threshold(gmt.lower_nc_threshold());
  mt.hysteresis = to_opae_threshold(gmt.hysteresis());

  return mt;
}

opaegrpc::fpga_remote_id *to_grpc_fpga_remote_id(const fpga_remote_id &id) {
  opaegrpc::fpga_remote_id *rid = new opaegrpc::fpga_remote_id();

  rid->set_allocated_hostname(new std::string(id.hostname));
  rid->set_unique_id(id.unique_id);

  return rid;
}

fpga_remote_id to_opae_fpga_remote_id(const opaegrpc::fpga_remote_id &gid) {
  fpga_remote_id id;
  const size_t len =
      std::min(gid.hostname().length(), (size_t)(HOST_NAME_MAX - 1));

  std::copy(gid.hostname().cbegin(), gid.hostname().cbegin() + len,
            id.hostname);
  id.hostname[len] = '\0';

  id.unique_id = gid.unique_id();

  return id;
}

void to_grpc_token_header(const fpga_token_header &hdr,
                          opaegrpc::fpga_token_header *gh) {
  gh->set_magic(hdr.magic);
  gh->set_vendor_id(hdr.vendor_id);
  gh->set_device_id(hdr.device_id);
  gh->set_segment(hdr.segment);
  gh->set_bus(hdr.bus);
  gh->set_device(hdr.device);
  gh->set_function(hdr.function);
  gh->set_interface(to_grpc_fpga_interface[hdr.interface]);
  gh->set_objtype(to_grpc_fpga_objtype[hdr.objtype]);
  gh->set_object_id(hdr.object_id);
  gh->set_allocated_guid(to_string(hdr.guid));
  gh->set_subsystem_vendor_id(hdr.subsystem_vendor_id);
  gh->set_subsystem_device_id(hdr.subsystem_device_id);
  gh->set_allocated_token_id(to_grpc_fpga_remote_id(hdr.token_id));
}

fpga_token_header to_opae_token_header(
    const opaegrpc::fpga_token_header &ghdr) {
  fpga_token_header h;

  h.magic = ghdr.magic();
  h.vendor_id = (uint16_t)ghdr.vendor_id();
  h.device_id = (uint16_t)ghdr.device_id();
  h.segment = (uint16_t)ghdr.segment();
  h.bus = (uint8_t)ghdr.bus();
  h.device = (uint8_t)ghdr.device();
  h.function = (uint8_t)ghdr.function();
  h.interface = to_opae_fpga_interface[ghdr.interface()];
  h.objtype = to_opae_fpga_objtype[ghdr.objtype()];
  h.object_id = ghdr.object_id();
  from_string(ghdr.guid(), h.guid);
  h.subsystem_vendor_id = (uint16_t)ghdr.subsystem_vendor_id();
  h.subsystem_device_id = (uint16_t)ghdr.subsystem_device_id();
  h.token_id = to_opae_fpga_remote_id(ghdr.token_id());

  return h;
}

opaegrpc::fpga_handle_header *to_grpc_handle_header(
    const fpga_handle_header &hdr) {
  opaegrpc::fpga_handle_header *ghdr = new opaegrpc::fpga_handle_header();

  ghdr->set_magic(hdr.magic);
  ghdr->set_allocated_token_id(to_grpc_fpga_remote_id(hdr.token_id));
  ghdr->set_allocated_handle_id(to_grpc_fpga_remote_id(hdr.handle_id));

  return ghdr;
}

fpga_handle_header to_opae_handle_header(
    const opaegrpc::fpga_handle_header &ghdr) {
  fpga_handle_header hdr;

  hdr.magic = ghdr.magic();
  hdr.token_id = to_opae_fpga_remote_id(ghdr.token_id());
  hdr.handle_id = to_opae_fpga_remote_id(ghdr.handle_id());

  return hdr;
}

inline bool is_valid(uint64_t bits, int pos) {
  return ((bits >> pos) & 1) != 0;
}

void to_grpc_fpga_properties(opaegrpc::fpga_properties *gprops,
                             const _fpga_properties *p) {
  gprops->set_magic(p->magic);
  gprops->set_valid_fields(p->valid_fields);

  const uint64_t bits = p->valid_fields;

  if (is_valid(bits, FPGA_PROPERTY_OBJTYPE))
    gprops->set_objtype(to_grpc_fpga_objtype[p->objtype]);

  if (is_valid(bits, FPGA_PROPERTY_SEGMENT)) gprops->set_segment(p->segment);

  if (is_valid(bits, FPGA_PROPERTY_BUS)) gprops->set_bus(p->bus);

  if (is_valid(bits, FPGA_PROPERTY_DEVICE)) gprops->set_device(p->device);

  if (is_valid(bits, FPGA_PROPERTY_FUNCTION)) gprops->set_function(p->function);

  if (is_valid(bits, FPGA_PROPERTY_SOCKETID))
    gprops->set_socket_id(p->socket_id);

  if (is_valid(bits, FPGA_PROPERTY_VENDORID))
    gprops->set_vendor_id(p->vendor_id);

  if (is_valid(bits, FPGA_PROPERTY_DEVICEID))
    gprops->set_device_id(p->device_id);

  if (is_valid(bits, FPGA_PROPERTY_GUID))
    gprops->set_allocated_guid(to_string(p->guid));

  if (is_valid(bits, FPGA_PROPERTY_OBJECTID))
    gprops->set_object_id(p->object_id);

  if (is_valid(bits, FPGA_PROPERTY_NUM_ERRORS))
    gprops->set_num_errors(p->num_errors);

  if (is_valid(bits, FPGA_PROPERTY_INTERFACE))
    gprops->set_interface(to_grpc_fpga_interface[p->interface]);

  if (is_valid(bits, FPGA_PROPERTY_SUB_VENDORID))
    gprops->set_subsystem_vendor_id(p->subsystem_vendor_id);

  if (is_valid(bits, FPGA_PROPERTY_SUB_DEVICEID))
    gprops->set_subsystem_device_id(p->subsystem_device_id);

  if (is_valid(bits, FPGA_PROPERTY_HOSTNAME))
    gprops->set_allocated_hostname(new std::string(p->hostname));

  if (is_valid(bits, FPGA_PROPERTY_OBJTYPE)) {
    const fpga_objtype objtype = p->objtype;

    if (objtype == FPGA_DEVICE) {
      if (is_valid(bits, FPGA_PROPERTY_NUM_SLOTS) ||
          is_valid(bits, FPGA_PROPERTY_BBSID) ||
          is_valid(bits, FPGA_PROPERTY_BBSVERSION)) {
        opaegrpc::fpga_properties::fpga_fields *device =
            new opaegrpc::fpga_properties::fpga_fields();

        if (is_valid(bits, FPGA_PROPERTY_NUM_SLOTS))
          device->set_num_slots(p->u.fpga.num_slots);

        if (is_valid(bits, FPGA_PROPERTY_BBSID))
          device->set_bbs_id(p->u.fpga.bbs_id);

        if (is_valid(bits, FPGA_PROPERTY_BBSVERSION))
          device->set_allocated_bbs_version(
              to_grpc_fpga_version(p->u.fpga.bbs_version));

        gprops->set_allocated_fpga(device);
      }

    } else {  // objtype == FPGA_ACCELERATOR

      if (is_valid(bits, FPGA_PROPERTY_ACCELERATOR_STATE) ||
          is_valid(bits, FPGA_PROPERTY_NUM_MMIO) ||
          is_valid(bits, FPGA_PROPERTY_NUM_INTERRUPTS)) {
        opaegrpc::fpga_properties::accelerator_fields *accelerator =
            new opaegrpc::fpga_properties::accelerator_fields();

        if (is_valid(bits, FPGA_PROPERTY_ACCELERATOR_STATE))
          accelerator->set_state(
              to_grpc_fpga_accelerator_state[p->u.accelerator.state]);

        if (is_valid(bits, FPGA_PROPERTY_NUM_MMIO))
          accelerator->set_num_mmio(p->u.accelerator.num_mmio);

        if (is_valid(bits, FPGA_PROPERTY_NUM_INTERRUPTS))
          accelerator->set_num_interrupts(p->u.accelerator.num_interrupts);

        gprops->set_allocated_accelerator(accelerator);
      }
    }
  }
}

fpga_properties to_opae_fpga_properties(
    const opaegrpc::fpga_properties &gprops) {
  _fpga_properties *p = opae_properties_create();
  if (!p) return p;

  const uint64_t bits = gprops.valid_fields();

  p->valid_fields = bits;

  if (is_valid(bits, FPGA_PROPERTY_OBJTYPE)) {
    if (gprops.has_objtype())
      p->objtype = to_opae_fpga_objtype[gprops.objtype()];
    else
      OPAE_ERR("fpga_properties missing objtype.");
  }

  if (is_valid(bits, FPGA_PROPERTY_SEGMENT)) {
    if (gprops.has_segment())
      p->segment = (uint16_t)gprops.segment();
    else
      OPAE_ERR("fpga_properties missing segment.");
  }

  if (is_valid(bits, FPGA_PROPERTY_BUS)) {
    if (gprops.has_bus())
      p->bus = (uint8_t)gprops.bus();
    else
      OPAE_ERR("fpga_properties missing bus.");
  }

  if (is_valid(bits, FPGA_PROPERTY_DEVICE)) {
    if (gprops.has_device())
      p->device = (uint8_t)gprops.device();
    else
      OPAE_ERR("fpga_properties missing device.");
  }

  if (is_valid(bits, FPGA_PROPERTY_FUNCTION)) {
    if (gprops.has_function())
      p->function = (uint8_t)gprops.function();
    else
      OPAE_ERR("fpga_properties missing function.");
  }

  if (is_valid(bits, FPGA_PROPERTY_SOCKETID)) {
    if (gprops.has_socket_id())
      p->socket_id = (uint8_t)gprops.socket_id();
    else
      OPAE_ERR("fpga_properties missing socket_id.");
  }

  if (is_valid(bits, FPGA_PROPERTY_VENDORID)) {
    if (gprops.has_vendor_id())
      p->vendor_id = (uint16_t)gprops.vendor_id();
    else
      OPAE_ERR("fpga_properties missing vendor_id.");
  }

  if (is_valid(bits, FPGA_PROPERTY_DEVICEID)) {
    if (gprops.has_device_id())
      p->device_id = (uint16_t)gprops.device_id();
    else
      OPAE_ERR("fpga_properties missing device_id.");
  }

  if (is_valid(bits, FPGA_PROPERTY_GUID)) {
    if (gprops.has_guid())
      from_string(gprops.guid(), p->guid);
    else
      OPAE_ERR("fpga_properties missing guid.");
  }

  if (is_valid(bits, FPGA_PROPERTY_OBJECTID)) {
    if (gprops.has_object_id())
      p->object_id = gprops.object_id();
    else
      OPAE_ERR("fpga_properties missing object_id.");
  }

  if (is_valid(bits, FPGA_PROPERTY_NUM_ERRORS)) {
    if (gprops.has_num_errors())
      p->num_errors = gprops.num_errors();
    else
      OPAE_ERR("fpga_properties missing num_errors.");
  }

  if (is_valid(bits, FPGA_PROPERTY_INTERFACE)) {
    if (gprops.has_interface())
      p->interface = to_opae_fpga_interface[gprops.interface()];
    else
      OPAE_ERR("fpga_properties missing interface.");
  }

  if (is_valid(bits, FPGA_PROPERTY_SUB_VENDORID)) {
    if (gprops.has_subsystem_vendor_id())
      p->subsystem_vendor_id = (uint16_t)gprops.subsystem_vendor_id();
    else
      OPAE_ERR("fpga_properties missing subsystem_vendor_id.");
  }

  if (is_valid(bits, FPGA_PROPERTY_SUB_DEVICEID)) {
    if (gprops.has_subsystem_device_id())
      p->subsystem_device_id = (uint16_t)gprops.subsystem_device_id();
    else
      OPAE_ERR("fpga_properties missing subsystem_device_id.");
  }

  if (is_valid(bits, FPGA_PROPERTY_HOSTNAME)) {
    if (gprops.has_hostname()) {
      size_t len =
          std::min(gprops.hostname().length(), (size_t)(HOST_NAME_MAX - 1));

      std::copy(gprops.hostname().cbegin(), gprops.hostname().cbegin() + len,
                p->hostname);
      p->hostname[len] = '\0';
    } else
      OPAE_ERR("fpga_properties missing hostname.");
  }

  if (is_valid(bits, FPGA_PROPERTY_OBJTYPE)) {
    const fpga_objtype objtype = to_opae_fpga_objtype[gprops.objtype()];

    if (objtype == FPGA_DEVICE) {
      if ((is_valid(bits, FPGA_PROPERTY_NUM_SLOTS) ||
           is_valid(bits, FPGA_PROPERTY_BBSID) ||
           is_valid(bits, FPGA_PROPERTY_BBSVERSION)) &&
          gprops.has_fpga()) {
        const opaegrpc::fpga_properties::fpga_fields &fpga = gprops.fpga();

        if (is_valid(bits, FPGA_PROPERTY_NUM_SLOTS)) {
          if (fpga.has_num_slots())
            p->u.fpga.num_slots = fpga.num_slots();
          else
            OPAE_ERR("opaegrpc::fpga_properties missing num_slots.");
        }

        if (is_valid(bits, FPGA_PROPERTY_BBSID)) {
          if (fpga.has_bbs_id())
            p->u.fpga.bbs_id = fpga.bbs_id();
          else
            OPAE_ERR("opaegrpc::fpga_properties missing bbs_id.");
        }

        if (is_valid(bits, FPGA_PROPERTY_BBSVERSION)) {
          if (fpga.has_bbs_version())
            p->u.fpga.bbs_version = to_opae_fpga_version(fpga.bbs_version());
          else
            OPAE_ERR("opaegrpc::fpga_properties missing bbs_version.");
        }
      }

    } else {  // objtype == FPGA_ACCELERATOR

      if ((is_valid(bits, FPGA_PROPERTY_ACCELERATOR_STATE) ||
           is_valid(bits, FPGA_PROPERTY_NUM_MMIO) ||
           is_valid(bits, FPGA_PROPERTY_NUM_INTERRUPTS)) &&
          gprops.has_accelerator()) {
        const opaegrpc::fpga_properties::accelerator_fields &accelerator =
            gprops.accelerator();

        if (is_valid(bits, FPGA_PROPERTY_ACCELERATOR_STATE)) {
          if (accelerator.has_state())
            p->u.accelerator.state =
                to_opae_fpga_accelerator_state[accelerator.state()];
          else
            OPAE_ERR("opaegrpc::fpga_properties missing state.");
        }

        if (is_valid(bits, FPGA_PROPERTY_NUM_MMIO)) {
          if (accelerator.has_num_mmio())
            p->u.accelerator.num_mmio = accelerator.num_mmio();
          else
            OPAE_ERR("opaegrpc::fpga_properties missing num_mmio.");
        }

        if (is_valid(bits, FPGA_PROPERTY_NUM_INTERRUPTS)) {
          if (accelerator.has_num_interrupts())
            p->u.accelerator.num_interrupts = accelerator.num_interrupts();
          else
            OPAE_ERR("opaegrpc::fpga_properties missing num_interrupts.");
        }
      }
    }
  }

  return p;
}

void to_grpc_fpga_properties(OPAEClient *client,
                             opaegrpc::fpga_properties *gprops,
                             const _fpga_properties *p) {
  UNUSED_PARAM(client);

  if (is_valid(p->valid_fields, FPGA_PROPERTY_PARENT)) {
    // On the client, p->parent will be a pointer to a _remote_token.
    _remote_token *rem = reinterpret_cast<_remote_token *>(p->parent);
    gprops->set_allocated_parent(to_grpc_fpga_remote_id(rem->hdr.token_id));
  }

  to_grpc_fpga_properties(gprops, p);
}

void to_grpc_fpga_properties(OPAEServiceImpl *server,
                             opaegrpc::fpga_properties *gprops,
                             const _fpga_properties *p) {
  UNUSED_PARAM(server);

  if (is_valid(p->valid_fields, FPGA_PROPERTY_PARENT)) {
    // On the server, p->parent will be a pointer to an opae_wrapped_token.
    opae_wrapped_token *wt = reinterpret_cast<opae_wrapped_token *>(p->parent);
    fpga_token_header *hdr =
        reinterpret_cast<fpga_token_header *>(wt->opae_token);
    gprops->set_allocated_parent(to_grpc_fpga_remote_id(hdr->token_id));
  }

  to_grpc_fpga_properties(gprops, p);
}

fpga_properties to_opae_fpga_properties(
    OPAEClient *client, const opaegrpc::fpga_properties &gprops) {
  _fpga_properties *p =
      reinterpret_cast<_fpga_properties *>(to_opae_fpga_properties(gprops));

  if (!p) return p;

  if (is_valid(p->valid_fields, FPGA_PROPERTY_PARENT) && gprops.has_parent()) {
    // On the client side, we translate from fpga_remote_id to
    // the corresponding _remote_token *.
    fpga_remote_id rid = to_opae_fpga_remote_id(gprops.parent());
    _remote_token *rtok;

    rtok = client->token_map_.find(rid);
    if (rtok)
      p->parent = rtok;
    else {
      p->parent = nullptr;
      OPAE_ERR("client failed to find token (%llu@%s)", rid.unique_id,
               rid.hostname);
    }
  }

  return p;
}

fpga_properties to_opae_fpga_properties(
    OPAEServiceImpl *server, const opaegrpc::fpga_properties &gprops) {
  _fpga_properties *p =
      reinterpret_cast<_fpga_properties *>(to_opae_fpga_properties(gprops));

  if (!p) return p;

  if (is_valid(p->valid_fields, FPGA_PROPERTY_PARENT) && gprops.has_parent()) {
    // On the server side, we translate from fpga_remote_id to
    // the corresponding fpga_token.
    fpga_remote_id rid = to_opae_fpga_remote_id(gprops.parent());
    fpga_token tok;

    tok = server->token_map_.find(rid);
    if (tok)
      p->parent = tok;
    else {
      p->parent = nullptr;
      OPAE_ERR("server failed to find token (%llu@%s)", rid.unique_id,
               rid.hostname);
    }
  }

  return p;
}

// Copyright(c) 2017, Intel Corporation
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

/**
* \file metrics.h
* \brief fpga metrics API
*/

#ifndef __FPGA_METRICS_INIT_H__
#define __FPGA_METRICS_INIT_H__

#include "vector.h"
#define IPERF                               "iperf"
#define DPERF                               "dperf"

#define PWRMGMT                            "power_mgmt"
#define THERLGMT                           "thermal_mgmt"
#define REVISION                           "revision"
#define PERF_FREEZE                         "freeze"
#define PERF_ENABLE                         "enable"
#define PERF_CACHE                          "cache"
#define PERF_FABRIC                         "fabric"
#define PERF_IOMMU                          "iommu"

#define PERF                          "performance"
struct DFH {
	union {
		uint64_t csr;
		struct {
			uint64_t id : 12;
			uint64_t  revision : 4;
			uint64_t next_header_offset : 24;
			uint64_t eol : 1;
			uint64_t reserved : 19;
			uint64_t  type : 4;
		};
	};
};

struct NEXT_AFU {
	union {
		uint64_t csr;
		struct {
			uint32_t next_afu : 24;
			uint64_t reserved : 40;
		};
	};
};

typedef struct {
	struct DFH dfh;
	uint64_t guid[2];
	struct NEXT_AFU next_afu;
} feature_definition;


struct metric_group_csr {
	union {
		uint64_t csr;
		struct {
			uint64_t reserved : 28;
			uint64_t units : 8;
			uint64_t group_id : 8;
			uint64_t eol : 1;
			uint64_t next_group_offset : 16;
			uint64_t reset : 1;
			uint64_t reset_access : 2;
		};
	};
};

struct metric_value_csr {
	union {
		uint64_t csr;
		struct {
			uint64_t reserved : 7;
			uint64_t eol : 1;
			uint64_t counter_id : 8;
			uint64_t value : 48;
		};
	};
};

fpga_result check_metrics_guid(const uint8_t *bitstream);

fpga_result discover_afu_metrics_feature(fpga_handle handle, uint64_t *offset);

fpga_result add_metric_vector(fpga_metric_vector *vector,
	uint64_t metric_id,
	const char *qualifier_name,
	const char *group_name,
	const char *group_sysfs,
	const char *metric_name,
	const char *metric_sysfs,
	const char *metric_units,
	enum fpga_metric_datatype  metric_datatype,
	enum fpga_metric_type	metric_type,
	enum fpga_hw_type	hw_type,
	uint64_t mmio_offset);

fpga_result enum_thermalmgmt_metrics(fpga_metric_vector *vector,
	 uint64_t *metric_id,
	char *sysfspath,
	 enum fpga_hw_type       hw_type);

	 fpga_result enum_powermgmt_metrics(fpga_metric_vector *vector,
		 uint64_t *metric_id,
		 char *sysfspath,
	 enum fpga_hw_type hw_type);

	 fpga_result enum_perf_counter_items(fpga_metric_vector *vector,
		 uint64_t *metric_id,
		 char *qualifier_name,
		 char *sysfspath,
		 char *sysfs_name,
	 enum fpga_metric_type metric_type,
	 enum fpga_hw_type  hw_type);

	 fpga_result enum_perf_counter_metrics(fpga_metric_vector *vector,
		 uint64_t *metric_id,
		 char *sysfspath,
	 enum fpga_hw_type  hw_type);

//fpga_result enum_fpga_metrics(struct _fpga_handle *_handle);

fpga_result sysfs_path_is_dir(const char *path);

fpga_result enum_fpga_metrics(fpga_handle handle);



fpga_result  get_metric_value_byid(struct _fpga_handle *_handle,fpga_metric_vector* enum_vector,
	uint64_t metric_id,
struct fpga_metric_t  *fpga_metric);

fpga_result  get_metricid_from_serach_string(const char *serach_string, fpga_metric_vector *fpga_enum_metrics_vecotr,
	uint64_t *metric_id);


fpga_result add_metric_info(struct _fpga_enum_metric* _enum_metrics, struct fpga_metric_t  *fpga_metric);

fpga_result delete_fpga_enum_metrics_vector(struct _fpga_handle *_handle);

///fpga_result  enum_bmc_metrics_info(fpga_token token, fpga_metric_vector *vector, uint64_t *metric_id, enum fpga_hw_type  hw_type);

fpga_result  enum_bmc_metrics_info(struct _fpga_handle *_handle, fpga_token token, fpga_metric_vector *vector, uint64_t *metric_id, enum fpga_hw_type  hw_type);



fpga_result enum_afu_metrics(fpga_handle handle,
	fpga_metric_vector *vector,
	uint64_t *metric_id,
	uint64_t metrics_offset);

fpga_result  get_afu_metric_value(fpga_handle handle,
	fpga_metric_vector* enum_vector,
	uint64_t metric_num,
	struct fpga_metric_t  *fpga_metric);

fpga_result get_fpga_object_type(fpga_handle handle, fpga_objtype *objtype);

fpga_result add_afu_metrics_vector(fpga_metric_vector *vector,
	uint64_t *metric_id,
	uint64_t group_value,
	uint64_t metric_value,
	uint64_t metric_offset);

#endif // __FPGA_METRICS_H__
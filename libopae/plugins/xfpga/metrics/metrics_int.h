// Copyright(c) 2018-2019, Intel Corporation
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
* \file metrics_int.h
* \brief fpga metrics utils functions
*/

#ifndef __FPGA_METRICS_INT_H__
#define __FPGA_METRICS_INT_H__

#include "vector.h"
#include "opae/metrics.h"
#include "metrics_metadata.h"
#include "metrics/bmc/bmc.h"
// Power,Thermal & Performance definations

#define PERF                               "*perf"


#define PWRMGMT                             "power_mgmt"
#define THERLGMT                            "thermal_mgmt"
#define REVISION                            "revision"
#define PERF_FREEZE                         "freeze"
#define PERF_ENABLE                         "enable"
#define PERF_CACHE                          "cache"
#define PERF_FABRIC                         "fabric"
#define PERF_IOMMU                          "iommu"
#define PERFORMANCE                         "performance"
#define FPGA_LIMIT                          "fpga_limit"
#define XEON_LIMIT                          "xeon_limit"
#define TEMP                                "Centigrade"

#define TEMPERATURE                         "Temperature"
#define VOLTAGE                             "Voltage"
#define CURRENT                             "Current"
#define POWER                               "Power"

#define MAX10_SYSFS_PATH                     "spi-altera.*.auto/spi_master/spi*/spi*.*"
#define MAX10_SENSOR_SYSFS_PATH              "spi-altera.*.auto/spi_master/spi*/spi*.*/sensor*"
#define SENSOR_SYSFS_NAME                    "name"
#define SENSOR_SYSFS_TYPE                    "type"
#define SENSOR_SYSFS_ID                      "id"
#define SENSOR_SYSFS_VALUE                   "value"
#define MILLI                                 1000

#define  FPGA_DISCRETE_VC_DEVICEID           0x0B30

#define  FPGA_DISCRETE_DC_DEVICEID           0x0B2B

#define BMC_LIB                             "libmodbmc.so"

// AFU DFH Struct
struct DFH {
	union {
		uint64_t csr;
		struct {
			uint64_t id:12;
			uint64_t  revision:4;
			uint64_t next_header_offset:24;
			uint64_t eol:1;
			uint64_t reserved:19;
			uint64_t  type:4;
		};
	};
};

struct NEXT_AFU {
	union {
		uint64_t csr;
		struct {
			uint32_t next_afu:24;
			uint64_t reserved:40;
		};
	};
};

typedef struct {
	struct DFH dfh;
	uint64_t guid[2];
	struct NEXT_AFU next_afu;
} feature_definition;

// metric group csr
struct metric_bbb_group {
	union {
		uint64_t csr;
		struct {
			uint64_t reserved:28;
			uint64_t units:8;
			uint64_t group_id:8;
			uint64_t eol:1;
			uint64_t next_group_offset:16;
			uint64_t reset:1;
			uint64_t reset_access:2;
		};
	};
};

// metric value csr
struct metric_bbb_value {
	union {
		uint64_t csr;
		struct {
			uint64_t reserved:7;
			uint64_t eol:1;
			uint64_t counter_id:8;
			uint64_t value:48;
		};
	};
};

// Metrics utils functions
fpga_result metric_sysfs_path_is_file(const char *path);

fpga_result metric_sysfs_path_is_dir(const char *path);

fpga_result add_metric_vector(fpga_metric_vector *vector,
				uint64_t metric_id,
				const char *qualifier_name,
				const char *group_name,
				const char *group_sysfs,
				const char *metric_name,
				const char *metric_sysfs,
				const char *metric_units,
				enum fpga_metric_datatype metric_datatype,
				enum fpga_metric_type	metric_type,
				enum fpga_hw_type	hw_type,
				uint64_t mmio_offset);

fpga_result enum_thermalmgmt_metrics(fpga_metric_vector *vector,
					uint64_t *metric_id,
					const char *sysfspath,
					enum fpga_hw_type hw_type);

fpga_result enum_powermgmt_metrics(fpga_metric_vector *vector,
					uint64_t *metric_id,
					const char *sysfspath,
					enum fpga_hw_type hw_type);

fpga_result enum_perf_counter_items(fpga_metric_vector *vector,
					uint64_t *metric_id,
					const char *qualifier_name,
					const char *sysfspath,
					const char *sysfs_name,
					enum fpga_metric_type metric_type,
					enum fpga_hw_type hw_type);

fpga_result enum_perf_counter_metrics(fpga_metric_vector *vector,
					uint64_t *metric_id,
					const char *sysfspath,
					enum fpga_hw_type  hw_type);

fpga_result enum_fpga_metrics(fpga_handle handle);


fpga_result get_fme_metric_value(fpga_handle handle,
				fpga_metric_vector *enum_vector,
				uint64_t metric_id,
				struct fpga_metric *fpga_metric);

fpga_result add_metric_info(struct _fpga_enum_metric *_enum_metrics,
				struct fpga_metric_info *fpga_metric_info);

fpga_result free_fpga_enum_metrics_vector(struct _fpga_handle *_handle);


fpga_result parse_metric_num_name(const char *search_string,
				fpga_metric_vector *fpga_enum_metrics_vector,
				uint64_t *metric_num);

fpga_result enum_bmc_metrics_info(struct _fpga_handle *_handle,
				fpga_metric_vector *vector,
				uint64_t *metric_id,
				enum fpga_hw_type hw_type);

fpga_result get_fpga_object_type(fpga_handle handle, fpga_objtype *objtype);

fpga_result get_pwr_thermal_value(const char *sysfs_path, uint64_t *value);

fpga_result clear_cached_values(fpga_handle handle);


fpga_result get_performance_counter_value(const char *group_sysfs,
					const char *metric_sysfs,
					uint64_t *value);

fpga_result get_bmc_metrics_values(fpga_handle handle,
				struct _fpga_enum_metric *_fpga_enum_metric,
				struct fpga_metric *fpga_metric);

// AFU Metric
fpga_result enum_afu_metrics(fpga_handle handle,
				fpga_metric_vector *vector,
				uint64_t *metric_id,
				uint64_t metrics_offset);

fpga_result get_afu_metric_value(fpga_handle handle,
				fpga_metric_vector	*enum_vector,
				uint64_t metric_num,
				struct fpga_metric *fpga_metric);

fpga_result add_afu_metrics_vector(fpga_metric_vector *vector,
				uint64_t *metric_id,
				uint64_t group_value,
				uint64_t metric_value,
				uint64_t metric_offset);

fpga_result discover_afu_metrics_feature(fpga_handle handle, uint64_t *offset);

fpga_result get_metric_data_info(const char *group_name,
		const char *metric_name,
		fpga_metric_metadata *metric_data_search,
		uint64_t size,
		fpga_metric_metadata *metric_data);

fpga_result xfpga_bmcLoadSDRs(struct _fpga_handle *_handle,
	bmc_sdr_handle *records,
	uint32_t *num_sensors);

fpga_result xfpga_bmcDestroySDRs(struct _fpga_handle *_handle,
	bmc_sdr_handle *records);

fpga_result xfpga_bmcReadSensorValues(struct _fpga_handle *_handle,
	bmc_sdr_handle records,
	bmc_values_handle *values,
	uint32_t *num_values);

fpga_result xfpga_bmcDestroySensorValues(struct _fpga_handle *_handle,
	bmc_values_handle *values);

fpga_result xfpga_bmcGetSensorReading(struct _fpga_handle *_handle,
	bmc_values_handle values,
	uint32_t sensor_number,
	uint32_t *is_valid,
	double *value);

fpga_result xfpga_bmcGetSDRDetails(struct _fpga_handle *_handle,
	bmc_values_handle values,
	uint32_t sensor_number,
	sdr_details *details);


#endif // __FPGA_METRICS_INT_H__

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

#ifndef __FPGA_METRICS_H__
#define __FPGA_METRICS_H__


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


fpga_result sysfs_path_is_dir(const char *path);

fpga_result add_metrics_vector(fpga_vector *vector,
								const char *metrics_class_name,
								const char *metrics_class_sysfs,
								const char *metrics_name,
								const char *metrics_sysfs,
								enum fpga_metrics_group  fpga_metrics_group,
								enum fpga_metrics_datatype  metrics_datatype,
								enum fpga_hw_type  hw_type);


fpga_result enum_fpga_metrics(struct _fpga_handle *_handle);

fpga_result enum_thermalmgmt_metrics(fpga_vector *vector,
									char *sysfspath,
									enum fpga_hw_type hw_type);

fpga_result enum_powermgmt_metrics(fpga_vector *vector,
									char *sysfspath,
									enum fpga_hw_type hw_type);

fpga_result enum_perf_counter_items(fpga_vector *vector,
									char *sysfspath,
									char *sysfs_name,
									enum fpga_metrics_group fpga_metrics_group,
									enum fpga_hw_type  hw_type);

fpga_result enum_perf_counter_metrics(fpga_vector *vector,
										char *sysfspath,
										enum fpga_hw_type  hw_type);



fpga_result delete_fpga_enum_metrics_vector(struct _fpga_handle *_handle);

fpga_result delete_fpga_metrics_vector(fpga_metrics  *fpga_metrics_values);

fpga_result  ParseMetricsSearchString(const char *serach_string,
	char *group_name,
	char *metrics_name);

fpga_result  EnumMetricsByString(char *group_name,
								char *name,
								fpga_vector *fpga_enum_metrics_vecotr,
								fpga_vector *fpga_filter_metrics_vecotr);


fpga_result  RemovieMetricsByString(char *group_name,
									char *name,
									fpga_vector *fpga_filter_metrics_vecotr);

fpga_result  getMetricsValuesByString(fpga_vector *fpga_filter_metrics_vecotr,
									fpga_metrics *fpga_metrics_vector);

fpga_result enumerate_metrcis_bmc(fpga_vector *vector,
								char *sysfs_path);

#endif // __FPGA_METRICS_H__
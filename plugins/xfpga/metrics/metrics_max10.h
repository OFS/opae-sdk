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
* \file metrics_max10.h
* \brief fpga metrics max10 functions
*/

#ifndef __FPGA_METRICS_MAX10_H__
#define __FPGA_METRICS_MAX10_H__


#define DFL_MAX10_SYSFS_PATH                     "dfl-fme*/spi-altera*/spi_master/spi*/spi*/*bmc-hwmon*/hwmon/hwmon*"
#define DFL_MAX10_SENSOR_SYSFS_PATH              "dfl-fme*/spi-altera*/spi_master/spi*/spi*/*bmc-hwmon*/hwmon/hwmon*/*_label"
#define DFL_TEMPERATURE                         "temp"
#define DFL_VOLTAGE                             "in"
#define DFL_CURRENT                             "curr"
#define DFL_POWER                               "power"
#define DFL_VALUE                               "input"


fpga_result read_sensor_sysfs_file(const char *sysfs, const char *file,
			void **buf, uint32_t *tot_bytes_ret);

fpga_result read_max10_value(struct _fpga_enum_metric *_fpga_enum_metric,
				double *dvalue);

fpga_result  dfl_enum_max10_metrics_info(struct _fpga_handle *_handle,
	fpga_metric_vector *vector,
	uint64_t *metric_num,
	enum fpga_hw_type  hw_type);
#endif // __FPGA_METRICS_MAX10_H__
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
* \file afu_metrics.c
* \brief Enumerates AFU Metrics BBB & retrives afu metrics values
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <uuid/uuid.h>

#include "xfpga.h"
#include "common_int.h"
#include "metrics_int.h"
#include "types_int.h"
#include "opae/metrics.h"
#include "metrics/vector.h"
#include "safe_string/safe_string.h"

// AFU BBB GUID
#define METRICS_BBB_GUID            "87816958-C148-4CD0-9D73-E8F258E9E3D7"
#define METRICS_BBB_ID_H            0x87816958C1484CD0
#define METRICS_BBB_ID_L            0x9D73E8F258E9E3D7

#define FEATURE_TYPE_BBB            0x2

#define METRIC_CSR_OFFSET           0x20
#define METRIC_NEXT_CSR             0x8


fpga_result discover_afu_metrics_feature(fpga_handle handle, uint64_t *offset)
{
	fpga_result result               = FPGA_OK;
	feature_definition feature_def;
	uint64_t bbs_offset              = 0;

	memset_s(&feature_def, sizeof(feature_def), 0);

	if (offset == NULL) {
		OPAE_ERR("Invalid Input Paramters");
		return FPGA_INVALID_PARAM;
	}

	// Read AFU DFH
	result = xfpga_fpgaReadMMIO64(handle, 0, 0x0, &(feature_def.dfh.csr));
	if (result != FPGA_OK) {
		OPAE_ERR("Invalid handle file descriptor");
		result = FPGA_NOT_SUPPORTED;
		return result;
	}

	// Serach for AFU Metrics BBB DFH
	while (feature_def.dfh.eol != 0 && feature_def.dfh.next_header_offset != 0) {

		bbs_offset = feature_def.dfh.next_header_offset;

		result = xfpga_fpgaReadMMIO64(handle, 0, feature_def.dfh.next_header_offset, &(feature_def.dfh.csr));
		if (result != FPGA_OK) {
			OPAE_ERR("Invalid handle file descriptor");
			result = FPGA_NOT_SUPPORTED;
			return result;
		}

		if (feature_def.dfh.type == FEATURE_TYPE_BBB) {

			result = xfpga_fpgaReadMMIO64(handle, 0, bbs_offset +0x8, &(feature_def.guid[0]));
			if (result != FPGA_OK) {
				OPAE_ERR("Invalid handle file descriptor");
				result = FPGA_NOT_SUPPORTED;
				return result;
			}

			result = xfpga_fpgaReadMMIO64(handle, 0, bbs_offset + 0x10, &(feature_def.guid[1]));
			if (result != FPGA_OK) {
				OPAE_ERR("Invalid handle file descriptor");
				result = FPGA_NOT_SUPPORTED;
				return result;
			}

			if (feature_def.guid[0] == METRICS_BBB_ID_L &&
				feature_def.guid[1] == METRICS_BBB_ID_H) {
				*offset = bbs_offset;
				return FPGA_OK;
			} else	{
				OPAE_ERR(" Metrics BBB Not Found \n ");
			}

		}

	}

	OPAE_ERR("AFU Metrics BBB Not Found \n ");
	return FPGA_NOT_FOUND;
}


fpga_result get_afu_metric_value(fpga_handle handle,
				fpga_metric_vector *enum_vector,
				uint64_t metric_num,
				struct fpga_metric *fpga_metric)
{
	fpga_result result                           = FPGA_OK;
	uint64_t index                               = 0;
	struct metric_bbb_value metric_csr;
	struct _fpga_enum_metric *_fpga_enum_metric  = NULL;
	uint64_t num_enun_metrics                    = 0;

	memset_s(&metric_csr, sizeof(metric_csr), 0);

	if (handle == NULL ||
		enum_vector == NULL ||
		fpga_metric == NULL) {
		OPAE_ERR("Invalid Input Paramters");
		return FPGA_INVALID_PARAM;
	}

	result = fpga_vector_total(enum_vector, &num_enun_metrics);
	if (result != FPGA_OK) {
		OPAE_ERR("Failed to get metric total");
		return FPGA_NOT_FOUND;
	}

	result = FPGA_NOT_FOUND;
	for (index = 0; index < num_enun_metrics; index++) {

		_fpga_enum_metric = (struct _fpga_enum_metric *)	fpga_vector_get(enum_vector, index);

		if (metric_num == _fpga_enum_metric->metric_num) {

			result = xfpga_fpgaReadMMIO64(handle, 0, _fpga_enum_metric->mmio_offset, &metric_csr.csr);

				fpga_metric->value.ivalue = metric_csr.value;
				result = FPGA_OK;

		}

	}

	return result;
}

fpga_result add_afu_metrics_vector(fpga_metric_vector *vector,
				  uint64_t *metric_id,
				  uint64_t group_value,
				  uint64_t metric_value,
				  uint64_t metric_offset)
{
	fpga_result result                      = FPGA_OK;
	struct metric_bbb_group group_csr;
	struct metric_bbb_value metric_csr;
	char group_name[SYSFS_PATH_MAX]         = { 0 };
	char metric_name[SYSFS_PATH_MAX]        = { 0 };
	char qualifier_name[SYSFS_PATH_MAX]     = { 0 };
	char metric_units[SYSFS_PATH_MAX]       = { 0 };

	memset_s(&group_csr, sizeof(group_csr), 0);
	memset_s(&metric_csr, sizeof(metric_csr), 0);

	if (metric_id == NULL ||
		vector == NULL) {
		OPAE_ERR("Invalid Input Paramters");
		return FPGA_INVALID_PARAM;
	}

	group_csr.csr = group_value;
	metric_csr.csr = metric_value;

	snprintf_s_i(group_name, sizeof(group_name), "%x", group_csr.group_id);
	snprintf_s_i(metric_name, sizeof(metric_name), "%x", metric_csr.counter_id);

	snprintf_s_si(qualifier_name, sizeof(qualifier_name), "%s:%x", "AFU", group_csr.group_id);
	snprintf_s_i(metric_units, sizeof(metric_units), "%x", group_csr.units);

	*metric_id = *metric_id + 1;

	result = add_metric_vector(vector, *metric_id, qualifier_name, group_name, "",
			metric_name, "", metric_units, FPGA_METRIC_DATATYPE_INT, FPGA_METRIC_TYPE_AFU, FPGA_HW_MCP, metric_offset);

	return result;
}


fpga_result enum_afu_metrics(fpga_handle handle,
			    fpga_metric_vector *vector,
			    uint64_t *metric_id,
			    uint64_t metrics_offset)
{
	fpga_result result                    = FPGA_NOT_FOUND;
	struct metric_bbb_group group_csr;
	struct metric_bbb_value metric_csr;
	uint64_t value_offset                 = 0;
	uint64_t group_offset                 = 0;

	memset_s(&group_csr, sizeof(group_csr), 0);
	memset_s(&metric_csr, sizeof(metric_csr), 0);

	if (handle == NULL ||
		vector == NULL ||
		metric_id == NULL) {
		OPAE_ERR("Invalid Input Paramters");
		return FPGA_INVALID_PARAM;
	}

	group_offset = metrics_offset + METRIC_CSR_OFFSET;

	while (true) {

			result = xfpga_fpgaReadMMIO64(handle, 0, group_offset, &group_csr.csr);

			if (group_csr.group_id != 0) {

				value_offset = group_offset + METRIC_NEXT_CSR;
				result = xfpga_fpgaReadMMIO64(handle, 0, value_offset, &metric_csr.csr);

				while (metric_csr.counter_id != 0) {

					// add to counter
					result = add_afu_metrics_vector(vector, metric_id, group_csr.csr, metric_csr.csr, value_offset);
					if (result != FPGA_OK) {
						OPAE_ERR("Failed to add metrics vector");
					}


					if (metric_csr.eol == 0) {
						value_offset = value_offset + METRIC_NEXT_CSR;
						result = xfpga_fpgaReadMMIO64(handle, 0, value_offset, &metric_csr.csr);
					} else {
						break;
					}

				} // end while

				if (group_offset == group_offset + group_csr.next_group_offset)
					break;

				group_offset = group_offset + group_csr.next_group_offset;

			} else {
				break;
			}

	} // end while

	return result;
}

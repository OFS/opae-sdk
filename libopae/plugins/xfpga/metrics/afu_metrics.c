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
* \file metrics_utils.c
* \brief fpga metrics API
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <uuid/uuid.h>

#include "common_int.h"
#include "metrics_int.h"
#include "types_int.h"
#include "opae/metrics.h"
#include "metrics/vector.h"
#include "metrics/bmc/bmc.h"
#include "safe_string/safe_string.h"

#define FEATURE_TYPE_BBB	         0x2
#define METRICS_BBB_GUID            "87816958-C148-4CD0-9D73-E8F258E9E3D7"
#define METRICS_BBB_ID_H            0x87816958C1484CD0
#define METRICS_BBB_ID_L            0x9D73E8F258E9E3D7


fpga_result discover_afu_metrics_feature(fpga_handle handle, uint64_t *offset)
{
	fpga_result result					= FPGA_OK;
	feature_definition feature_def		= {0};
	uint64_t bbs_offset					= 0;

	if (offset == NULL) {
		FPGA_ERR("Invlaid Input Paramters");
		return FPGA_INVALID_PARAM;
	}

	result = fpgaReadMMIO64(handle, 0, 0x0, &(feature_def.dfh.csr));
	if (result != FPGA_OK) {
		FPGA_ERR("Invalid handle file descriptor");
		result = FPGA_NOT_SUPPORTED;
		return result;
	}

	//printf("feature_def.dfh.next_header_offset =%llx \n", feature_def.dfh.next_header_offset);

	// Serach for mertics BBB
	while (feature_def.dfh.eol != 0 && feature_def.dfh.next_header_offset != 0) {

		bbs_offset = feature_def.dfh.next_header_offset;

		result = fpgaReadMMIO64(handle, 0, feature_def.dfh.next_header_offset, &(feature_def.dfh.csr));
		if (result != FPGA_OK) {
			FPGA_ERR("Invalid handle file descriptor");
			result = FPGA_NOT_SUPPORTED;
			return result;
		}

		//printf("feature_def.dfh.next_header_offset =%llx \n", feature_def.dfh.next_header_offset);

		if (feature_def.dfh.type == FEATURE_TYPE_BBB) {

			//printf("feature_def.dfh.next_header_offset =%llx \n", feature_def.dfh.next_header_offset);

			result = fpgaReadMMIO64(handle, 0, bbs_offset +0x8, &(feature_def.guid[0]));
			if (result != FPGA_OK) {
				FPGA_ERR("Invalid handle file descriptor");
				result = FPGA_NOT_SUPPORTED;
				return result;
			}

			result = fpgaReadMMIO64(handle, 0, bbs_offset + 0x10, &(feature_def.guid[1]));
			if (result != FPGA_OK) {
				FPGA_ERR("Invalid handle file descriptor");
				result = FPGA_NOT_SUPPORTED;
				return result;
			}
			//printf("feature_def.guid[0] =%llx \n", feature_def.guid[0]);
			//printf("feature_def.guid[1] =%llx \n", feature_def.guid[1]);
			//printf("feature_def.dfh.next_header_offset =%llx \n", feature_def.dfh.next_header_offset);
			if (feature_def.guid[0] == METRICS_BBB_ID_L &&
				feature_def.guid[1] == METRICS_BBB_ID_H) {

				*offset = bbs_offset;
				printf("Metics BBS FOUND \n ");
				return FPGA_OK;
			} else	{
				FPGA_ERR(" Metrics BBS Not Found \n ");
			}

		}

	}
	return FPGA_NOT_FOUND;
}


fpga_result  get_afu_metric_value(fpga_handle handle,
								fpga_metric_vector *enum_vector,
								uint64_t metric_num,
								struct fpga_metric_t *fpga_metric)
{
	fpga_result result								= FPGA_OK;
	uint64_t index									= 0;
	struct metric_value_csr value_csr				= { 0 };
	struct _fpga_enum_metric	*_fpga_enum_metric	= NULL;

	if (handle == NULL ||
		enum_vector == NULL ||
		fpga_metric == NULL) {
		FPGA_ERR("Invlaid Input Paramters");
		return FPGA_INVALID_PARAM;
	}

	for (index = 0; index < fpga_vector_total(enum_vector); index++) {

		_fpga_enum_metric = (struct _fpga_enum_metric *)	fpga_vector_get(enum_vector, index);

		if (metric_num == _fpga_enum_metric->metric_num) {

			result = fpgaReadMMIO64(handle, 0, _fpga_enum_metric->mmio_offset, &value_csr.csr);

				//printf("get_afu_metric_value_byid value_csr.csr =%llx \n", value_csr.csr);
				fpga_metric->value.ivalue = value_csr.value;

		}

	}
//

	return result;
}

fpga_result add_afu_metrics_vector(fpga_metric_vector *vector,
									uint64_t *metric_id,
									uint64_t group_value,
									uint64_t metric_value,
									uint64_t metric_offset)
{
	fpga_result result						= FPGA_OK;
	struct metric_group_csr group_csr		= { 0 };
	struct metric_value_csr metric_csr		= { 0 };
	char group_name[SYSFS_PATH_MAX]			= { 0 };
	char metric_name[SYSFS_PATH_MAX]		= { 0 };
	char qualifier_name[SYSFS_PATH_MAX]		= { 0 };
	char metric_units[SYSFS_PATH_MAX]		= { 0 };

	if (metric_id == NULL ||
		vector == NULL) {
		FPGA_ERR("Invlaid Input Paramters");
		return FPGA_INVALID_PARAM;
	}

	group_csr.csr = group_value;
	metric_csr.csr = metric_value;

	sprintf(group_name, "%x", group_csr.group_id);
	sprintf(metric_name, "%x", metric_csr.counter_id);

	sprintf(qualifier_name, "%s:%x:%x", "AFU", group_csr.group_id, metric_csr.counter_id);
	sprintf(metric_units, "%x", group_csr.units);

	*metric_id = *metric_id + 1;

	add_metric_vector(vector, *metric_id, qualifier_name, group_name, "",
		metric_name, "", metric_units, FPGA_METRIC_DATATYPE_INT, FPGA_METRIC_TYPE_AFU, FPGA_HW_MCP, metric_offset);

	return result;
}


fpga_result enum_afu_metrics(fpga_handle handle,
							fpga_metric_vector *vector,
							uint64_t *metric_id,
							uint64_t metrics_offset)
{
	fpga_result result					= FPGA_NOT_FOUND;
	struct metric_group_csr group_csr	= { 0 };
	struct metric_value_csr metric_csr	= { 0 };
	uint64_t value_offset				= 0;
	uint64_t group_offset				= 0;

	if (handle == NULL ||
		vector == NULL ||
		metric_id == NULL) {
		FPGA_ERR("Invlaid Input Paramters");
		return FPGA_INVALID_PARAM;
	}

	value_offset = metrics_offset + 0x20;
	group_offset = metrics_offset + 0x20;

	while (true) {

			result = fpgaReadMMIO64(handle, 0, group_offset, &group_csr.csr);

			//printf("group_csr.csr =%llx \n", group_csr.csr);

			if (group_csr.group_id != 0) {

				value_offset = group_offset + 0x8;
				result = fpgaReadMMIO64(handle, 0, value_offset, &metric_csr.csr);

				//printf("value_csr.csr =%llx \n", metric_csr.csr);

				while (metric_csr.counter_id != 0) {

					// add to counter
					result = add_afu_metrics_vector(vector, metric_id, group_csr.csr, metric_csr.csr, value_offset);
					if (result != FPGA_OK) {
						FPGA_ERR("Failed to add metrics vector");
					}


					if (metric_csr.eol == 0) {
						value_offset = value_offset + 0x8;
						result = fpgaReadMMIO64(handle, 0, value_offset, &metric_csr.csr);
						//printf("value_csr.csr =%llx \n", metric_csr.csr);
					} else {
						//printf("VALUE Break \n");
						break;
					}

				} // end while

				if (group_offset == group_offset + group_csr.next_group_offset)
					break;

				group_offset = group_offset + group_csr.next_group_offset;

				//printf("offset = %llx\n", group_offset);

			} else {
				//printf("group Break \n");
				break;
			}

	} // end while

	//printf(" ----------------value =%llx \n", value);

	return result;
}
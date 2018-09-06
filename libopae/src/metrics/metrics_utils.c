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


#include "common_int.h"
#include "metrics_int.h"
#include "types_int.h"
#include "opae/metrics.h"
#include "metrics/vector.h"
#include "bmcinfo.h"
#include "bmcdata.h"

#include "safe_string/safe_string.h"


fpga_result  ParseMetricsSearchString(const char *serach_string,
									char *group_name,
									char *metrics_name)
{
	fpga_result result			= FPGA_OK;
	size_t init_size			= 0;
	char *str					= NULL;
	//errno_t e					= 0;

	printf("ParseMetricsSearchString ENTER \n");

	if (serach_string == NULL ||
		group_name == NULL ||
		metrics_name == NULL) {
		FPGA_ERR("Invlaid Input Paramters");
		return FPGA_INVALID_PARAM;
	}

	str = strstr(serach_string, ":");
	if (str == NULL) {
		FPGA_ERR("Invlaid Input Paramters");
		return FPGA_INVALID_PARAM;
	}


	/*
	printf("str== %s\n", str);
	printf("str+1== %s\n", str + 1);
	printf("name== %s\n", name);
	printf("name== %ld\n", strlen(str + 1));*/
	/*

	e = strncpy_s(fpga_enum_metrics->metrics_class_name, sizeof(fpga_enum_metrics->metrics_class_name),
		metrics_class_name, SYSFS_PATH_MAX);*/

	strncpy(metrics_name, str + 1, strlen(str + 1));

	init_size = strcspn(serach_string, ":");

	strncpy(group_name, serach_string, init_size);

	group_name[init_size] = '\0';

	//printf("serach_string-> %s\n", serach_string);
	//printf("metrics_name-> %s\n", metrics_name);
	//printf("group_name-> %s\n", group_name);

	return result;
}

fpga_result add_metrics_vector(fpga_vector *vector,
								const char *group_name,
								const char *group_sysfs,
								const char *metrics_name,
								const char *metrics_sysfs,
								enum fpga_metrics_group  fpga_metrics_group,
								enum fpga_metrics_datatype  metrics_datatype,
								enum fpga_hw_type  hw_type)
{

	fpga_result result                                = FPGA_OK;
	struct _fpga_enum_metrics *fpga_enum_metrics      = NULL;
	errno_t e                                         = 0;

	/*
	printf(" perfctr_class_name              :%s \n", metrics_class_name);
	printf(" perfctr_class_sysfs_path        :%s \n", metrics_class_sysfs);
	printf(" perfctr_name                    :%s \n", metrics_name);
	printf(" perfctr_sysfs_path              :%s \n", metrics_sysfs);
	*/

	if (vector == NULL) {
		FPGA_ERR("Invlaid Input Paramters");
		return FPGA_INVALID_PARAM;
	}

	if ((group_name == NULL) ||
		(group_sysfs == NULL) ||
		(metrics_name == NULL) ||
		(metrics_sysfs == NULL)) {

		FPGA_ERR("Invlaid Input Paramters");
		return FPGA_INVALID_PARAM;
	}


	fpga_enum_metrics = (struct _fpga_enum_metrics *)malloc(sizeof(struct _fpga_enum_metrics));
	if (fpga_enum_metrics == NULL) {
		FPGA_ERR("Failed to allocate memory");
		return FPGA_NO_MEMORY;
	}


	e = strncpy_s(fpga_enum_metrics->metrics_class_name, sizeof(fpga_enum_metrics->metrics_class_name),
		group_name, SYSFS_PATH_MAX);
	if (EOK != e)
		goto out_free;

	e = strncpy_s(fpga_enum_metrics->metrics_class_sysfs, sizeof(fpga_enum_metrics->metrics_class_sysfs),
		group_sysfs, SYSFS_PATH_MAX);
	if (EOK != e)
		goto out_free;



	e = strncpy_s(fpga_enum_metrics->metrics_name, sizeof(fpga_enum_metrics->metrics_name),
		metrics_name, SYSFS_PATH_MAX);
	if (EOK != e)
		goto out_free;

	e = strncpy_s(fpga_enum_metrics->metrics_sysfs, sizeof(fpga_enum_metrics->metrics_sysfs),
		metrics_sysfs, SYSFS_PATH_MAX);
	if (EOK != e)
		goto out_free;


	fpga_enum_metrics->fpga_metrics_group = fpga_metrics_group;
	fpga_enum_metrics->metrics_datatype = metrics_datatype;
	fpga_enum_metrics->hw_type = hw_type;

	fpga_vector_push(vector, fpga_enum_metrics);

	return result;

out_free:
	free(fpga_enum_metrics);
	return FPGA_INVALID_PARAM;
}


fpga_result check_duplicate_metrics_vector(fpga_vector *vector,
											const char *group_name,
											const char *metrics_name)
{
	fpga_result result								= FPGA_OK;
	struct _fpga_enum_metrics *fpga_metrics			= NULL;
	uint64_t i										= 0;

	if ((group_name == NULL) ||
		(metrics_name == NULL)) {
		FPGA_MSG("Invalid token within handle");
		return FPGA_INVALID_PARAM;
	}

	for (i = 0; i < fpga_vector_total(vector); i++) {
		fpga_metrics = (struct _fpga_enum_metrics *) fpga_vector_get(vector, i);

		if (((strcasecmp(fpga_metrics->metrics_class_name, group_name) == 0) &&
			(strcasecmp(fpga_metrics->metrics_name, metrics_name) == 0))) {
			return FPGA_NOT_SUPPORTED;
		}
	}

	return result;
}

fpga_result  EnumMetricsByString(char *group_name,
								char *metrics_name,
								fpga_vector *fpga_enum_metrics_vecotr,
								fpga_vector *fpga_filter_metrics_vecotr)

{
	fpga_result result							= FPGA_OK;
	struct _fpga_enum_metrics *fpga_metrics		= NULL;
	uint64_t i									= 0;

	for (i = 0; i < fpga_vector_total((fpga_enum_metrics_vecotr)); i++) {
		fpga_metrics = (struct _fpga_enum_metrics *)	fpga_vector_get((fpga_enum_metrics_vecotr), i);

		if (((strcasecmp(fpga_metrics->metrics_class_name, group_name) == 0) &&
			(strcasecmp(fpga_metrics->metrics_name, metrics_name) == 0)) ||
			((strcasecmp(fpga_metrics->metrics_class_name, group_name) == 0) &&
			(strcasecmp(metrics_name, "*") == 0)) ||

			((strcasecmp(group_name, "*") == 0) &&
			(strcasecmp(metrics_name, "*") == 0))) {

			result = check_duplicate_metrics_vector(fpga_filter_metrics_vecotr, group_name, metrics_name);
			if (result != FPGA_OK) {
				FPGA_MSG("Duplicate");
				return result;
			}

			result = add_metrics_vector(fpga_filter_metrics_vecotr,
										fpga_metrics->metrics_class_name,
										fpga_metrics->metrics_class_sysfs,
										fpga_metrics->metrics_name,
										fpga_metrics->metrics_sysfs,
										fpga_metrics->fpga_metrics_group,
										fpga_metrics->metrics_datatype,
										fpga_metrics->hw_type);


		}


	} // end of for loop


	return result;
}




fpga_result add_metrics_vector_values(fpga_metrics *vector,
										const char *group_name,
										const char *metrics_name,
										enum fpga_metrics_datatype  metrics_datatype,
										enum fpga_hw_type  hw_type,
										fpga_values values)
{
	fpga_metrics_values *metrics_info_val = NULL;
	fpga_result result = FPGA_OK;
	errno_t e;

	/*
	printf(" perfctr_class_name              :%s \n", group_name);
	printf(" perfctr_name                    :%s \n", metrics_name);

	*/

	if (vector == NULL) {
		FPGA_ERR("Invlaid Input Paramters");
		return FPGA_INVALID_PARAM;
	}


	if ((group_name == NULL) ||
		(metrics_name == NULL)) {

		FPGA_ERR("Invalid token within handle");
		return FPGA_INVALID_PARAM;
	}


	metrics_info_val = (struct fpga_metrics_values *)malloc(sizeof(struct fpga_metrics_values));
	if (metrics_info_val == NULL) {
		FPGA_ERR("Failed to allocate memory");
		return FPGA_NO_MEMORY;
	}


	e = strncpy_s(metrics_info_val->group_name, sizeof(metrics_info_val->group_name),
		group_name, SYSFS_PATH_MAX);
	if (EOK != e)
		goto out_free;



	e = strncpy_s(metrics_info_val->metrics_name, sizeof(metrics_info_val->metrics_name),
		metrics_name, SYSFS_PATH_MAX);
	if (EOK != e)
		goto out_free;

	metrics_info_val->metrics_datatype = metrics_datatype;
	metrics_info_val->hw_type = hw_type;

	if (metrics_datatype == FPAG_METRICS_INT) {

		metrics_info_val->fpga_value.ivalue = values.ivalue;

	} else if (metrics_datatype == FPAG_METRICS_FLAOT) {

		metrics_info_val->fpga_value.fvalue = values.fvalue;
	} else {
		metrics_info_val->fpga_value.dvalue = values.dvalue;
	}

	fpga_metrics_vector_push(vector, metrics_info_val);

	return result;

out_free:
	free(metrics_info_val);
	return FPGA_INVALID_PARAM;
}

fpga_result  getMetricsValuesByString(fpga_vector *fpga_filter_metrics_vecotr,
										fpga_metrics *fpga_metrics_vector)
{
	fpga_result result									= FPGA_OK;
	char sysfs_path[SYSFS_PATH_MAX]						= { 0 };
	struct _fpga_enum_metrics *fpga_metrics				= NULL;
	uint64_t val										= 0;
	uint64_t i											= 0;
	Values *vptr										= NULL;
	Values *vals										= NULL;
	bool bmc_read										= false;

	fpga_values value;

	for (i = 0; i < fpga_vector_total((fpga_filter_metrics_vecotr)); i++) {
		fpga_metrics = (struct _fpga_enum_metrics *)	fpga_vector_get((fpga_filter_metrics_vecotr), i);


		if ((fpga_metrics->hw_type == FPAG_HW_DCP_RC) &&
			((fpga_metrics->fpga_metrics_group == FPAG_GROUP_POWER) ||
			(fpga_metrics->fpga_metrics_group == FPAG_GROUP_TEMP))) {

			//   printf("-------fpga_metrics->metrics_class_name =%s   \n",fpga_metrics->metrics_class_name);
			//   printf("-------fpga_metrics->metrics_class_name =%s   \n",fpga_metrics->metrics_name);

			if (!bmc_read) {
				bmc_read = true;
				bmc_read_sensor_data("/sys/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0/", &vals);
			}


				for (vptr = vals; NULL != vptr; vptr = vptr->next) {
					if (!(((SDR_SENSOR_IS_TEMP(vptr)))
						|| (SDR_SENSOR_IS_POWER(vptr))))
						continue;


								if (vptr->is_valid) {

									if (strcasecmp(fpga_metrics->metrics_name, vptr->name) == 0)  {

										//printf("-------MATCH fpga_metrics->metrics_class_name =%s   \n", fpga_metrics->metrics_name);

										if (vptr->val_type == SENSOR_INT) {
											value.ivalue = vptr->value.i_val;
										} else if (vptr->val_type == SENSOR_FLOAT) {
											value.fvalue = vptr->value.f_val;
										}


										add_metrics_vector_values(fpga_metrics_vector,
																fpga_metrics->metrics_class_name,
																fpga_metrics->metrics_name,
																fpga_metrics->metrics_datatype,
																fpga_metrics->hw_type,
																value);

									}


								} // valid


				} // end of if
			continue;


		} // ennd BMC IF


			if (fpga_metrics->fpga_metrics_group == FPAG_GROUP_PERF_CACHE) {

				snprintf_s_ss(sysfs_path, sizeof(sysfs_path), "%s/%s", fpga_metrics->metrics_class_sysfs, PERF_FREEZE);

				// Read Cache Freeze
				result = sysfs_read_u64(sysfs_path, &val);
				if (result != FPGA_OK) {
							FPGA_ERR("Failed to read perf cache");
							return result;
				}

				if (val != 0x1) {
						// Write Cache Freeze
						result = sysfs_write_u64(sysfs_path, 0x1);
						if (result != FPGA_OK) {
							FPGA_ERR("Failed to clear port errors");
							return result;
						}

				}

			} // end cache

			if (fpga_metrics->fpga_metrics_group == FPAG_GROUP_PERF_IOMMU) {

				snprintf_s_ss(sysfs_path, sizeof(sysfs_path), "%s/%s", fpga_metrics->metrics_class_sysfs, PERF_FREEZE);

				// Read IOMMU Freeze
				result = sysfs_read_u64(sysfs_path, &val);
				if (result != FPGA_OK) {
						FPGA_ERR("Failed to read perf iommu");
						return result;
				}

				if (val != 0x1) {
					// Writer IOMMU Freeze
					result = sysfs_write_u64(sysfs_path, 0x1);
					if (result != FPGA_OK) {
						FPGA_ERR("Failed to write perf iommu");
						return result;
				}

				}

			} // end iommu

			if (fpga_metrics->fpga_metrics_group == FPAG_GROUP_PERF_FABRIC) {

				snprintf_s_ss(sysfs_path, sizeof(sysfs_path), "%s/%s", fpga_metrics->metrics_class_sysfs, PERF_ENABLE);

				// Read Fabric Enable
				result = sysfs_read_u64(sysfs_path, &val);
				if (result != FPGA_OK) {
					FPGA_ERR("Failed to read perf fabric enable");
					return result;
				}

				if (val != 0x1) {
					// Writer Fabric Enable
					result = sysfs_write_u64(sysfs_path, 0x1);
					if (result != FPGA_OK) {
					FPGA_ERR("Failed to read perf fabric enable");
					return result;
					}

				}

				snprintf_s_ss(sysfs_path, sizeof(sysfs_path), "%s/%s", fpga_metrics->metrics_class_sysfs, PERF_FREEZE);

				// Read Fabric Freeze
				result = sysfs_read_u64(sysfs_path, &val);
				if (result != FPGA_OK) {
						FPGA_ERR("Failed to read perf fabric freeze");
						return result;
				}

				if (val != 0x1) {
					// Write Fabric Freeze
					result = sysfs_write_u64(sysfs_path, 0x1);
					if (result != FPGA_OK) {
						FPGA_ERR("Failed to write perf fabric freeze");
						return result;
					}

				}

			} // end fabric


			result = sysfs_read_u64(fpga_metrics->metrics_sysfs, &value.ivalue);
			if (result != FPGA_OK) {
				FPGA_ERR("Failed to read Metrics values");
				return result;
			}


			if (strcasecmp(fpga_metrics->metrics_name, "fpga_limit") == 0)  {
				value.ivalue = value.ivalue / 8;
			}

			if (strcasecmp(fpga_metrics->metrics_name, "xeon_limit") == 0)  {
				value.ivalue = value.ivalue / 8;
			}



			if (fpga_metrics->fpga_metrics_group == FPAG_GROUP_PERF_CACHE) {

				snprintf_s_ss(sysfs_path, sizeof(sysfs_path), "%s/%s", fpga_metrics->metrics_class_sysfs, PERF_FREEZE);

				// Read Cache Freeze
				result = sysfs_read_u64(sysfs_path, &val);
				if (result != FPGA_OK) {
					FPGA_ERR("Failed to read perf cache");
					return result;
				}

				if (val != 0x0) {
					// Write Cache Freeze
					result = sysfs_write_u64(sysfs_path, 0x0);
					if (result != FPGA_OK) {
						FPGA_ERR("Failed to write perf cache");
						return result;
					}

				}

			} // end cache


			if (fpga_metrics->fpga_metrics_group == FPAG_GROUP_PERF_IOMMU) {

				snprintf_s_ss(sysfs_path, sizeof(sysfs_path), "%s/%s", fpga_metrics->metrics_class_sysfs, PERF_FREEZE);

				// Read IOMMU Freeze
				result = sysfs_read_u64(sysfs_path, &val);
				if (result != FPGA_OK) {
					FPGA_ERR("Failed to read perf iommu");
					return result;
				}

				if (val != 0x0) {
					// Writer IOMMU Freeze
					result = sysfs_write_u64(sysfs_path, 0x0);
					if (result != FPGA_OK) {
						FPGA_ERR("Failed to write perf iommu");
						return result;
					}

				}

			} // end iommu


			if (fpga_metrics->fpga_metrics_group == FPAG_GROUP_PERF_FABRIC) {

				snprintf_s_ss(sysfs_path, sizeof(sysfs_path), "%s/%s", fpga_metrics->metrics_class_sysfs, PERF_ENABLE);

				// Read Fabric Enable
				result = sysfs_read_u64(sysfs_path, &val);
				if (result != FPGA_OK) {
					FPGA_ERR("Failed to read perf fabric enable");
					return result;
				}
				/*
				if (val != 0x0) {
					// Writer Fabric Enable 
					result = sysfs_write_u64(sysfs_path, 0x0);
					if (result != FPGA_OK) {
						FPGA_ERR("Failed to write perf fabric enable");
					//	return result;
					}

				}*/

				snprintf_s_ss(sysfs_path, sizeof(sysfs_path), "%s/%s", fpga_metrics->metrics_class_sysfs, PERF_FREEZE);

				// Read Fabric Freeze
				result = sysfs_read_u64(sysfs_path, &val);
				if (result != FPGA_OK) {
					FPGA_ERR("Failed to read perf fabric freeze");
					return result;
				}

				if (val != 0x0) {
					// Write Fabric Freeze
					result = sysfs_write_u64(sysfs_path, 0x0);
					if (result != FPGA_OK) {
						FPGA_ERR("Failed to write perf fabric freeze");
						return result;
					}

				}

			} // end fabric



			printf("&&&& %-20s       | %-40s        |%-40ld      \n", fpga_metrics->metrics_class_name,
				fpga_metrics->metrics_name,
				value.ivalue);

			add_metrics_vector_values(fpga_metrics_vector,
				fpga_metrics->metrics_class_name,
				fpga_metrics->metrics_name,
				fpga_metrics->metrics_datatype,
				fpga_metrics->hw_type,
				value);
	}


	return result;
}


fpga_result sysfs_path_is_dir(const char *path)
{
	struct stat astats;

	if (path == NULL) {
		return FPGA_INVALID_PARAM;
	}

	if ((stat(path, &astats)) != 0) {
		printf("stat() failed\n");
		return FPGA_NOT_FOUND;
	}

	if (S_ISDIR(astats.st_mode)) {
		return FPGA_OK;
	}

	return FPGA_NOT_FOUND;
}



fpga_result enum_thermalmgmt_metrics(fpga_vector *vector,
									char *sysfspath,
									enum fpga_hw_type  hw_type)
{
	fpga_result result							= FPGA_OK;
	DIR *dir									= NULL;
	struct dirent *dirent						= NULL;
	char sysfs_path[SYSFS_PATH_MAX]				= { 0 };
	char metrics_sysfs_path[SYSFS_PATH_MAX]		= { 0 };


	if (vector == NULL) {
		FPGA_ERR("Invlaid Input Paramters");
		return FPGA_INVALID_PARAM;
	}

	if (sysfspath == NULL) {
		FPGA_ERR("Invlaid Input Paramters");
		return FPGA_INVALID_PARAM;
	}

	snprintf_s_ss(sysfs_path, sizeof(sysfs_path), "%s/%s", sysfspath, THERLGMT);

	dir = opendir(sysfs_path);
	if (NULL == dir) {
		FPGA_MSG("can't find dir %s ", strerror(errno));
		return FPGA_NOT_FOUND;
	}

	while ((dirent = readdir(dir)) != NULL) {
		if (!strcmp(dirent->d_name, "."))
			continue;
		if (!strcmp(dirent->d_name, ".."))
			continue;
		if (!strcmp(dirent->d_name, REVISION))
			continue;

		//printf(" \n \n  ---- dirent->d_name =%s \n", dirent->d_name);

		snprintf_s_ss(metrics_sysfs_path, sizeof(metrics_sysfs_path), "%s/%s", sysfs_path, dirent->d_name);

		result = add_metrics_vector(vector, THERLGMT, sysfs_path, dirent->d_name, metrics_sysfs_path, FPAG_GROUP_TEMP, FPAG_METRICS_INT, hw_type);
		if (result != FPGA_OK) {
			FPGA_MSG("Failed to add metrics");
			return result;
		}

	}

	return result;
}


fpga_result enum_powermgmt_metrics(fpga_vector *vector,
									char *sysfspath,
									enum fpga_hw_type hw_type)
{
	fpga_result result							= FPGA_OK;
	DIR *dir									= NULL;
	struct dirent *dirent						= NULL;
	char sysfs_path[SYSFS_PATH_MAX]				= { 0 };
	char metrics_sysfs_path[SYSFS_PATH_MAX]		= { 0 };

	if (vector == NULL) {
		FPGA_ERR("Invlaid Input Paramters");
		return FPGA_INVALID_PARAM;
	}

	if (sysfspath == NULL) {
		FPGA_ERR("Invlaid Input Paramters");
		return FPGA_INVALID_PARAM;
	}

	snprintf_s_ss(sysfs_path, sizeof(sysfs_path), "%s/%s", sysfspath, PWRMGMT);

	dir = opendir(sysfs_path);
	if (NULL == dir) {
		FPGA_MSG("can't find dir %s ", strerror(errno));
		return FPGA_NOT_FOUND;
	}

	while ((dirent = readdir(dir)) != NULL) {
		if (!strcmp(dirent->d_name, "."))
			continue;
		if (!strcmp(dirent->d_name, ".."))
			continue;
		if (!strcmp(dirent->d_name, REVISION))
			continue;

		snprintf_s_ss(metrics_sysfs_path, sizeof(metrics_sysfs_path), "%s/%s", sysfs_path, dirent->d_name);

		result = add_metrics_vector(vector, PWRMGMT, sysfs_path, dirent->d_name, metrics_sysfs_path, FPAG_GROUP_POWER, FPAG_METRICS_INT, hw_type);
		if (result != FPGA_OK) {
			FPGA_MSG("Failed to add metrics");
			return result;
		}

	}

	return result;
}



fpga_result enum_perf_counter_items(fpga_vector *vector,
										char *sysfspath,
										char *sysfs_name,
										enum fpga_metrics_group fpga_metrics_group,
										enum fpga_hw_type  hw_type)
{
	fpga_result result							= FPGA_OK;
	DIR *dir									= NULL;
	struct dirent *dirent						= NULL;
	char sysfs_path[SYSFS_PATH_MAX]				= { 0 };
	char metrics_sysfs_path[SYSFS_PATH_MAX]		= { 0 };

	if (vector == NULL) {
		FPGA_ERR("Invlaid Input Paramters");
		return FPGA_INVALID_PARAM;
	}

	if ((sysfspath == NULL) ||
		(sysfs_name == NULL)) {
		FPGA_ERR("Invlaid Input Paramters");
		return FPGA_INVALID_PARAM;
	}


	snprintf_s_ss(sysfs_path, sizeof(sysfs_path), "%s/%s", sysfspath, sysfs_name);

	dir = opendir(sysfs_path);
	if (NULL == dir) {
		FPGA_MSG("can't find dir %s ", strerror(errno));
		return FPGA_NOT_FOUND;
	}

	while ((dirent = readdir(dir)) != NULL) {
		if (!strcmp(dirent->d_name, "."))
			continue;
		if (!strcmp(dirent->d_name, ".."))
			continue;
		if (!strcmp(dirent->d_name, PERF_ENABLE))
			continue;

		if (!strcmp(dirent->d_name, PERF_FREEZE))
			continue;
		if (dirent->d_type == DT_DIR)
			continue;

		snprintf_s_ss(metrics_sysfs_path, sizeof(metrics_sysfs_path), "%s/%s", sysfs_path, dirent->d_name);

		result = add_metrics_vector(vector, sysfs_name, sysfs_path, dirent->d_name, metrics_sysfs_path, fpga_metrics_group, FPAG_METRICS_INT, hw_type);
		if (result != FPGA_OK) {
			FPGA_MSG("Failed to add metrics");
			return result;
		}

	}

	return result;

}
fpga_result enum_perf_counter_metrics(fpga_vector *vector,
											char *sysfspath,
											enum fpga_hw_type  hw_type)
{
	fpga_result result						= FPGA_OK;
	DIR *dir								= NULL;
	struct dirent *dirent					= NULL;
	char sysfs_path[SYSFS_PATH_MAX]			= { 0 };
	char sysfs_ipath[SYSFS_PATH_MAX]		= { 0 };
	char sysfs_dpath[SYSFS_PATH_MAX]		= { 0 };

	if (vector == NULL) {
		FPGA_ERR("Invlaid Input Paramters");
		return FPGA_INVALID_PARAM;
	}

	if (sysfspath == NULL) {
		FPGA_ERR("Invlaid Input Paramters");
		return FPGA_INVALID_PARAM;
	}

	snprintf_s_ss(sysfs_ipath, sizeof(sysfs_ipath), "%s/%s", sysfspath, IPERF);
	snprintf_s_ss(sysfs_dpath, sizeof(sysfs_dpath), "%s/%s", sysfspath, DPERF);


	if (sysfs_path_is_dir(sysfs_ipath) == FPGA_OK) {

		snprintf_s_ss(sysfs_path, sizeof(sysfs_path), "%s/%s", sysfspath, IPERF);

	} else if (sysfs_path_is_dir(sysfs_dpath) == FPGA_OK) {

		snprintf_s_ss(sysfs_path, sizeof(sysfs_path), "%s/%s", sysfspath, DPERF);

	} else {

		FPGA_MSG("NO Perf Counters");
		return FPGA_NOT_FOUND;
	}

	dir = opendir(sysfs_path);
	if (NULL == dir) {
		FPGA_MSG("can't find dirt %s ", strerror(errno));
		return FPGA_NOT_FOUND;
	}

	while ((dirent = readdir(dir)) != NULL) {

		if (!strcmp(dirent->d_name, "."))
			continue;
		if (!strcmp(dirent->d_name, ".."))
			continue;
		if (!strcmp(dirent->d_name, REVISION))
			continue;

		if (strcmp(dirent->d_name, PERF_CACHE) == 0) {
			result = enum_perf_counter_items(vector, sysfs_path, dirent->d_name, FPAG_GROUP_PERF_CACHE, hw_type);
			if (result != FPGA_OK) {
				FPGA_MSG("Failed to add metrics");
			}

		}

		if (strcmp(dirent->d_name, PERF_FABRIC) == 0) {
			result = enum_perf_counter_items(vector, sysfs_path, dirent->d_name, FPAG_GROUP_PERF_FABRIC, hw_type);
			if (result != FPGA_OK) {
				FPGA_MSG("Failed to add metrics");
			}

		}

		if (strcmp(dirent->d_name, PERF_IOMMU) == 0) {
			result = enum_perf_counter_items(vector, sysfs_path, dirent->d_name, FPAG_GROUP_PERF_IOMMU, hw_type);
			if (result != FPGA_OK) {
				FPGA_MSG("Failed to add metrics");
			}

		}

	}

	return result;
}

fpga_result delete_fpga_enum_metrics_vector(struct _fpga_handle *_handle)
{
	fpga_result result    = FPGA_OK;
	uint64_t i = 0;

	if (_handle == NULL) {
		FPGA_ERR("Invalid handle ");
		return FPGA_INVALID_PARAM;
	}

	if (_handle->magic != FPGA_HANDLE_MAGIC) {
		FPGA_MSG("Invalid handle");
		return FPGA_INVALID_PARAM;
	}


	for (i = 0; i < fpga_vector_total(&(_handle->fpga_enum_metrics_vector)); i++) {
		fpga_vector_delete(&(_handle->fpga_enum_metrics_vector), i);
	}

	fpga_vector_free(&(_handle->fpga_enum_metrics_vector));

	return result;
}

fpga_result delete_fpga_metrics_vector(fpga_metrics *fpga_metrics_values)
{
	fpga_result result = FPGA_OK;
	uint64_t i = 0;

	if (fpga_metrics_values == NULL) {
		FPGA_MSG("Invalid handle");
		return FPGA_INVALID_PARAM;
	}

	for (i = 0; i < fpga_metrics_vector_total(fpga_metrics_values); i++) {
		fpga_metrics_vector_delete(fpga_metrics_values, i);
	}

	fpga_metrics_vector_free(fpga_metrics_values);

	return result;

}

fpga_result enumerate_metrcis_bmc(fpga_vector *vector,
								char *sysfs_path)
{
	fpga_result result = FPGA_OK;
	Values *vals = NULL;
	Values *vptr;
	enum fpga_hw_type  hw_type = FPAG_HW_DCP_RC;
	enum fpga_metrics_datatype  metrics_datatype = FPAG_METRICS_INT;
	enum fpga_metrics_group  fpga_metrics_group = FPAG_GROUP_TEMP;
	char metrics_class_name[SYSFS_PATH_MAX];

	if (NULL == sysfs_path)
		return FPGA_INVALID_PARAM;

	bmc_read_sensor_data(sysfs_path, &vals);

	for (vptr = vals; NULL != vptr; vptr = vptr->next) {
		if (!(((SDR_SENSOR_IS_TEMP(vptr)))
			|| (SDR_SENSOR_IS_POWER(vptr))))
			continue;

		if (vptr->is_valid) {

			if (SDR_SENSOR_IS_TEMP(vptr)) {
				fpga_metrics_group = FPAG_GROUP_TEMP;

				strncpy_s(metrics_class_name, sizeof(metrics_class_name),
					THERLGMT, SYSFS_PATH_MAX);


			} else if (SDR_SENSOR_IS_POWER(vptr)) {
				fpga_metrics_group = FPAG_GROUP_POWER;
				strncpy_s(metrics_class_name, sizeof(metrics_class_name),
					PWRMGMT, SYSFS_PATH_MAX);
			}


			if (vptr->val_type == SENSOR_INT) {
				//printf("%" PRIu64 " %ls", vptr->value.i_val, vptr->units);
				metrics_datatype = FPAG_METRICS_INT;

			} else if (vptr->val_type == SENSOR_FLOAT) {
				//printf("%.2lf %ls", vptr->value.f_val, vptr->units);
				metrics_datatype = FPAG_METRICS_FLAOT;
			}

			result = add_metrics_vector(vector, metrics_class_name, "", vptr->name, "", fpga_metrics_group, metrics_datatype, hw_type);
			if (result != FPGA_OK) {
				FPGA_MSG("Failed to add metrics");
				return result;
			}

		}


	}



	return result;

}


fpga_result enum_fpga_metrics(struct _fpga_handle *_handle)
{
	fpga_result result					= FPGA_OK;
	struct _fpga_token  *_token			= NULL;
	uint64_t deviceid					= 0;
	// uint64_t i						= 0;


	if (_handle == NULL) {
		FPGA_ERR("Invalid handle ");
		return FPGA_INVALID_PARAM;
	}

	_token = (struct _fpga_token *)_handle->token;
	if (_token == NULL) {
		FPGA_ERR("Invalid token within handle");
		return FPGA_INVALID_PARAM;
	}

	// get fpga device id.
	result = get_fpga_deviceid(_handle, &deviceid);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed to read device id.");
		return result;
	}

	// Init vector
	result = fpga_vector_init(&(_handle->fpga_enum_metrics_vector));
	if (result != FPGA_OK) {
		FPGA_ERR("Failed to init vector");
		return result;
	}


	switch (deviceid) {
		// MCP
	case FPGA_INTEGRATED_DEVICEID: {

			result = enum_powermgmt_metrics(&(_handle->fpga_enum_metrics_vector), _token->sysfspath, FPAG_HW_MCP);
			if (result != FPGA_OK) {
				FPGA_MSG("Failed to Enum Power metrics.");
			}

			result = enum_thermalmgmt_metrics(&(_handle->fpga_enum_metrics_vector), _token->sysfspath, FPAG_HW_MCP);
			if (result != FPGA_OK) {
				FPGA_MSG("Failed to Enum Thermal metrics.");
			}

			result = enum_perf_counter_metrics(&(_handle->fpga_enum_metrics_vector), _token->sysfspath, FPAG_HW_MCP);
			if (result != FPGA_OK) {
				FPGA_MSG("Failed to Enum Perforamnce metrics.");
			}

		}
		break;

		// DCP RC
	case FPGA_DISCRETE_DEVICEID: {

				result = enum_perf_counter_metrics(&(_handle->fpga_enum_metrics_vector), _token->sysfspath, FPAG_HW_DCP_RC);
				if (result != FPGA_OK) {
					FPGA_MSG("Failed to Enum Perforamnce metrics.");
				}

				enumerate_metrcis_bmc(&(_handle->fpga_enum_metrics_vector), _token->sysfspath);

		}
		break;

	default:
		FPGA_MSG("Unknown Device ID.");
	}


	/*

	struct _fpga_enum_metrics* fpga_enum_metrics = NULL;
	for (uint64_t i = 0; i < fpga_vector_total(&(_handle->fpga_enum_metrics_vector)); i++) {
		fpga_enum_metrics = (struct _fpga_enum_metrics*)	fpga_vector_get(&(_handle->fpga_enum_metrics_vector), i);

		printf("metrics_class_name : %s\n ", fpga_enum_metrics->metrics_class_name);
		printf("metrics_class_sysfs : %s\n ", fpga_enum_metrics->metrics_class_sysfs);
		printf("metrics_name : %s\n ", fpga_enum_metrics->metrics_name);
		printf("metrics_sysfs : %s\n ", fpga_enum_metrics->metrics_sysfs);
		printf("\n");

	}

	printf("total %ld \n ", fpga_vector_total(&(_handle->fpga_enum_metrics_vector)));
	*/


	return result;

}


fpga_result  RemovieMetricsByString(char *group_name,
									char *name,
									fpga_vector *fpga_filter_metrics_vecotr)

{
	fpga_result result = FPGA_OK;
	struct _fpga_enum_metrics *fpga_metrics = NULL;
	uint64_t i = 0;


	for (i = 0; i < fpga_vector_total((fpga_filter_metrics_vecotr)); i++) {
		fpga_metrics = (struct _fpga_enum_metrics *)	fpga_vector_get((fpga_filter_metrics_vecotr), i);


		if (((strcasecmp(fpga_metrics->metrics_class_name, group_name) == 0) &&
			(strcasecmp(fpga_metrics->metrics_name, name) == 0)) ||
			((strcasecmp(fpga_metrics->metrics_class_name, group_name) == 0) &&
			(strcasecmp(name, "*") == 0)) ||

			((strcasecmp(group_name, "*") == 0) &&
			(strcasecmp(name, "*") == 0))) {

			fpga_vector_delete(fpga_filter_metrics_vecotr, i);


		}


	}


	return result;
}
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
// POSSIBILITY OF SUCH DAMAG

/**
* \file metrics_metadata.h
* \brief fpga metrics utils functions
*/

#ifndef __FPGA_MAX10_METADATA_H__
#define __FPGA_MAX10_METADATA_H__

#include <stdio.h>
#include <string.h>
#include "opae/fpga.h"


#define MAX10_MDATA_SIZE 22

fpga_metric_metadata fpga_max10_metric_metadata[] = {

	// POWER 
	{ .group_name = "power_mgmt", .metric_name = "Board Power",
		.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "Watts" },

	{ .group_name = "power_mgmt", .metric_name = "12V Backplane Current",
		.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "Amps" },

	{ .group_name = "power_mgmt", .metric_name = "12V Backplane Voltage",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "Volts" },

	{ .group_name = "power_mgmt", .metric_name = "1.2V Voltage",
		.data_type = FPGA_METRIC_DATATYPE_BOOL, .metric_units = "Volts" },

	{ .group_name = "power_mgmt", .metric_name = "1.8V Voltage",
	.data_type = FPGA_METRIC_DATATYPE_BOOL, .metric_units = "Volts" },

	{ .group_name = "power_mgmt", .metric_name = "3.3V Voltage",
	.data_type = FPGA_METRIC_DATATYPE_BOOL, .metric_units = "Volts" },

	{ .group_name = "power_mgmt", .metric_name = "FPGA Core Voltage",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "Volts" },

	{ .group_name = "power_mgmt", .metric_name = "FPGA Core Current",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "Amps" },

	{.group_name = "power_mgmt", .metric_name = "12V AUX Current",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "Amps" },

	{.group_name = "power_mgmt", .metric_name = "12V AUX Voltage",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "Volts" },

	{.group_name = "power_mgmt", .metric_name = "QSFP0 Supply Voltage",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "Volts" },


	{.group_name = "power_mgmt", .metric_name = "12V AUX Current",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "Amps" },

	{.group_name = "power_mgmt", .metric_name = "12V AUX Current",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "Amps" },

	{.group_name = "power_mgmt", .metric_name = "QSFP1 Supply Voltage",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "Volts" },

	// THERMAL
	{ .group_name = "thermal_mgmt", .metric_name = "FPGA Die Temperature",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "Centigrade" },

	{ .group_name = "thermal_mgmt", .metric_name = "Board Temperature",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "Centigrade" },

	{.group_name = "thermal_mgmt", .metric_name = "QSFP0 Temperature",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "Centigrade" },

	{ .group_name = "thermal_mgmt", .metric_name = "QSFP1 Temperature",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "Centigrade" },

	{ .group_name = "thermal_mgmt", .metric_name = "PKVL0 Core Temperature",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "Centigrade" },

	{ .group_name = "thermal_mgmt", .metric_name = "PKVL0 SerDes Temperature",
	.data_type = FPGA_METRIC_DATATYPE_BOOL, .metric_units = "Centigrade " },

	{ .group_name = "thermal_mgmt", .metric_name = "PKVL1 Core Temperature",
	.data_type = FPGA_METRIC_DATATYPE_BOOL, .metric_units = "Centigrade" },

	{ .group_name = "thermal_mgmt", .metric_name = "PKVL1 SerDes Temperature",
	.data_type = FPGA_METRIC_DATATYPE_BOOL, .metric_units = "Centigrade" },

};

#endif //__FPGA_MAX10_METADATA_H__
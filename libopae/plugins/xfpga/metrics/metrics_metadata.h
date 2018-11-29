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

#ifndef __FPGA_METRICS_METADATA_H__
#define __FPGA_METRICS_METADATA_H__

#include <stdio.h>
#include <string.h>
#include "opae/fpga.h"

typedef struct fpga_metric_metadata {

	char group_name[FPGA_METRIC_STR_SIZE];
	char metric_name[FPGA_METRIC_STR_SIZE];
	char qualifier_name[FPGA_METRIC_STR_SIZE];

	enum fpga_metric_datatype data_type;
	enum fpga_metric_type metric_type;
	char metric_units[FPGA_METRIC_STR_SIZE];
	uint64_t mmio_offset;

	uint32_t range_start;
	uint32_t range_end;

} fpga_metric_metadata;

#define MCP_MDATA_SIZE 61

fpga_metric_metadata mcp_metric_metadata[] = {

	{ .group_name = "power_mgmt", .metric_name = "consumed",
		.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "Watts" },

	{ .group_name = "power_mgmt", .metric_name = "threshold1",
		.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "Watts" },

	{ .group_name = "power_mgmt", .metric_name = "threshold2",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "Watts" },

	{ .group_name = "power_mgmt", .metric_name = "threshold1_status",
		.data_type = FPGA_METRIC_DATATYPE_BOOL, .metric_units = "" },

	{ .group_name = "power_mgmt", .metric_name = "threshold2_status",
	.data_type = FPGA_METRIC_DATATYPE_BOOL, .metric_units = "" },

	{ .group_name = "power_mgmt", .metric_name = "rtl",
	.data_type = FPGA_METRIC_DATATYPE_BOOL, .metric_units = "" },

	{ .group_name = "power_mgmt", .metric_name = "fpga_limit",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "Watts" },

	{ .group_name = "power_mgmt", .metric_name = "xeon_limit",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "Watts" },

	// THERMAL
	{ .group_name = "thermal_mgmt", .metric_name = "temperature",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "Centigrade" },

	{ .group_name = "thermal_mgmt", .metric_name = "threshold1",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "Centigrade" },

	{ .group_name = "thermal_mgmt", .metric_name = "threshold2",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "Centigrade" },

	{ .group_name = "thermal_mgmt", .metric_name = "threshold_trip",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "Centigrade" },

	{ .group_name = "thermal_mgmt", .metric_name = "threshold1_reached",
	.data_type = FPGA_METRIC_DATATYPE_BOOL, .metric_units = "" },

	{ .group_name = "thermal_mgmt", .metric_name = "threshold2_reached",
	.data_type = FPGA_METRIC_DATATYPE_BOOL, .metric_units = "" },

	{ .group_name = "thermal_mgmt", .metric_name = "threshold1_policy",
	.data_type = FPGA_METRIC_DATATYPE_BOOL, .metric_units = "" },

	// pef cache
	{ .group_name = "performance", .metric_name = "clock",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "Hz" },

	{ .group_name = "performance:cache", .metric_name = "data_write_port_contention",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	{ .group_name = "performance:cache", .metric_name = "hold_request",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	{ .group_name = "performance:cache", .metric_name = "read_hit",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	{ .group_name = "performance:cache", .metric_name = "read_miss",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	{ .group_name = "performance:cache", .metric_name = "rx_eviction",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	{ .group_name = "performance:cache", .metric_name = "rx_req_stall",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	{ .group_name = "performance:cache", .metric_name = "tag_write_port_contention",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	{ .group_name = "performance:cache", .metric_name = "tx_req_stall",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	{ .group_name = "performance:cache", .metric_name = "write_hit",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	{ .group_name = "performance:cache", .metric_name = "write_miss",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	// pef fabric

	{ .group_name = "performance:fabric", .metric_name = "mmio_read",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	{ .group_name = "performance:fabric", .metric_name = "mmio_write",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	{ .group_name = "performance:fabric", .metric_name = "pcie0_read",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	{ .group_name = "performance:fabric", .metric_name = "pcie0_write",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	{ .group_name = "performance:fabric", .metric_name = "pcie1_read",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	{ .group_name = "performance:fabric", .metric_name = "pcie1_write",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	{ .group_name = "performance:fabric", .metric_name = "upi_read",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	{ .group_name = "performance:fabric", .metric_name = "upi_write",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	// perf fabric port0

	{ .group_name = "performance:fabric:port0", .metric_name = "mmio_read",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	{ .group_name = "performance:fabric:port0", .metric_name = "mmio_write",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	{ .group_name = "performance:fabric:port0", .metric_name = "pcie0_read",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	{ .group_name = "performance:fabric:port0", .metric_name = "pcie0_write",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	{ .group_name = "performance:fabric:port0", .metric_name = "pcie1_read",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	{ .group_name = "performance:fabric:port0", .metric_name = "pcie1_write",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	{ .group_name = "performance:fabric:port0", .metric_name = "upi_read",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	{ .group_name = "performance:fabric:port0", .metric_name = "upi_write",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },


	// perf iommu
	{ .group_name = "performance:iommu", .metric_name = "iotlb_1g_hit",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	{ .group_name = "performance:iommu", .metric_name = "iotlb_1g_miss",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	{ .group_name = "performance:iommu", .metric_name = "iotlb_2m_hit",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },\

	{ .group_name = "performance:iommu", .metric_name = "iotlb_2m_miss",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	{ .group_name = "performance:iommu", .metric_name = "iotlb_4k_hit",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	{ .group_name = "performance:iommu", .metric_name = "iotlb_4k_miss",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	{ .group_name = "performance:iommu", .metric_name = "rcc_hit",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	{ .group_name = "performance:iommu", .metric_name = "rcc_miss",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	{ .group_name = "performance:iommu", .metric_name = "slpwc_l3_hit",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	{ .group_name = "performance:iommu", .metric_name = "slpwc_l3_miss",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	{ .group_name = "performance:iommu", .metric_name = "slpwc_l4_hit",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	{ .group_name = "performance:iommu", .metric_name = "slpwc_l4_miss",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },


	// perf iommu afu0
	{ .group_name = "performance:iommu:afu0", .metric_name = "devtlb_1g_fill",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	{ .group_name = "performance:iommu:afu0", .metric_name = "devtlb_2m_fill",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	{ .group_name = "performance:iommu:afu0", .metric_name = "devtlb_4k_fill",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	{ .group_name = "performance:iommu:afu0", .metric_name = "devtlb_read_hit",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	{ .group_name = "performance:iommu:afu0", .metric_name = "devtlb_write_hit",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	{ .group_name = "performance:iommu:afu0", .metric_name = "read_transaction",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	{ .group_name = "performance:iommu:afu0", .metric_name = "write_transaction",
	.data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" }

};

#endif //__FPGA_METRICS_METADATA_H__

#ifndef __FPGA_METRICS_METADATA_H__
#define __FPGA_METRICS_METADATA_H__

#include <stdio.h>
#include <string.h>
#include "opae/fpga.h"

typedef struct fpga_metric_metadata_t
{
	char group_name[256];
	char metric_name[256];
	enum fpga_metric_datatype data_type;
	char metric_units[256];
	int offset;

}fpga_metric_metadata;

#define MCP_MDATA_SIZE 62

fpga_metric_metadata mcp_metric_metadata[] =
{
	{ .group_name = "power_mgmt", .metric_name = "consumed", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "Watts" },
	{ .group_name = "power_mgmt", .metric_name = "threshold1", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "Watts" },
	{ .group_name = "power_mgmt", .metric_name = "threshold2", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "Watts" },
	{ .group_name = "power_mgmt", .metric_name = "threshold1_status", .data_type = FPGA_METRIC_DATATYPE_BOOL, .metric_units = "" },
	{ .group_name = "power_mgmt", .metric_name = "threshold2_status", .data_type = FPGA_METRIC_DATATYPE_BOOL, .metric_units = "" },
	{ .group_name = "power_mgmt", .metric_name = "rtl", .data_type = FPGA_METRIC_DATATYPE_BOOL, .metric_units = "" },
	{ .group_name = "power_mgmt", .metric_name = "fpga_limit", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "Watts" },
	{ .group_name = "power_mgmt", .metric_name = "xeon_limit", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "Watts" },

	// THERMAL
	{ .group_name = "thermal_mgmt", .metric_name = "temperature", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "Centigrade" },
	{ .group_name = "thermal_mgmt", .metric_name = "threshold1", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "Centigrade" },
	{ .group_name = "thermal_mgmt", .metric_name = "threshold2", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "Centigrade" },
	{ .group_name = "thermal_mgmt", .metric_name = "threshold_trip", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "Centigrade" },
	{ .group_name = "thermal_mgmt", .metric_name = "threshold1_reached", .data_type = FPGA_METRIC_DATATYPE_BOOL, .metric_units = "" },
	{ .group_name = "thermal_mgmt", .metric_name = "threshold2_reached", .data_type = FPGA_METRIC_DATATYPE_BOOL, .metric_units = "" },
	{ .group_name = "thermal_mgmt", .metric_name = "threshold1_policy", .data_type = FPGA_METRIC_DATATYPE_BOOL, .metric_units = "" },

	// pef cache
	{ .group_name = "performance", .metric_name = "clock", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "Hz" },

	{ .group_name = "performance:cache", .metric_name = "data_write_port_contention", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },
	{ .group_name = "performance:cache", .metric_name = "hold_request", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },
	{ .group_name = "performance:cache", .metric_name = "read_hit", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },
	{ .group_name = "performance:cache", .metric_name = "read_miss", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },
	{ .group_name = "performance:cache", .metric_name = "rx_eviction", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },
	{ .group_name = "performance:cache", .metric_name = "rx_req_stall", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },
	{ .group_name = "performance:cache", .metric_name = "tag_write_port_contention", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },
	{ .group_name = "performance:cache", .metric_name = "tx_req_stall", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },
	{ .group_name = "performance:cache", .metric_name = "write_hit", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },
	{ .group_name = "performance:cache", .metric_name = "write_miss", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	// pef pabric

	{ .group_name = "performance:fabric", .metric_name = "mmio_read", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },
	{ .group_name = "performance:fabric", .metric_name = "mmio_write", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	{ .group_name = "performance:fabric", .metric_name = "pcie0_read", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },
	{ .group_name = "performance:fabric", .metric_name = "pcie0_write", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	{ .group_name = "performance:fabric", .metric_name = "pcie1_read", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },
	{ .group_name = "performance:fabric", .metric_name = "pcie1_write", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	{ .group_name = "performance:fabric", .metric_name = "upi_read", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },
	{ .group_name = "performance:fabric", .metric_name = "upi_write", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	// pef fabric port0

	{ .group_name = "performance:fabric:port0", .metric_name = "mmio_read", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },
	{ .group_name = "performance:fabric:port0", .metric_name = "mmio_write", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	{ .group_name = "performance:fabric:port0", .metric_name = "pcie0_read", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },
	{ .group_name = "performance:fabric:port0", .metric_name = "pcie0_write", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	{ .group_name = "performance:fabric:port0", .metric_name = "pcie1_read", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },
	{ .group_name = "performance:fabric:port0", .metric_name = "pcie1_write", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	{ .group_name = "performance:fabric:port0", .metric_name = "upi_read", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },
	{ .group_name = "performance:fabric:port0", .metric_name = "upi_write", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },


	// pef iommu
	{ .group_name = "performance:iommu", .metric_name = "iotlb_1g_hit", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },
	{ .group_name = "performance:iommu", .metric_name = "iotlb_1g_miss", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	{ .group_name = "performance:iommu", .metric_name = "iotlb_2m_hit", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },
	{ .group_name = "performance:iommu", .metric_name = "iotlb_2m_miss", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	{ .group_name = "performance:iommu", .metric_name = "iotlb_4k_hit", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },
	{ .group_name = "performance:iommu", .metric_name = "iotlb_4k_miss", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	{ .group_name = "performance:iommu", .metric_name = "rcc_hit", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },
	{ .group_name = "performance:iommu", .metric_name = "rcc_miss", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	{ .group_name = "performance:iommu", .metric_name = "slpwc_l3_hit", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },
	{ .group_name = "performance:iommu", .metric_name = "slpwc_l3_miss", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	{ .group_name = "performance:iommu", .metric_name = "slpwc_l4_hit", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },
	{ .group_name = "performance:iommu", .metric_name = "slpwc_l4_miss", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },


	// pef iommu afu0
	{ .group_name = "performance:iommu:afu0", .metric_name = "devtlb_1g_fill", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },
	{ .group_name = "performance:iommu:afu0", .metric_name = "devtlb_2m_fill", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	{ .group_name = "performance:iommu:afu0", .metric_name = "devtlb_4k_fill", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },
	{ .group_name = "performance:iommu:afu0", .metric_name = "devtlb_read_hit", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	{ .group_name = "performance:iommu:afu0", .metric_name = "devtlb_write_hit", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },
	{ .group_name = "performance:iommu:afu0", .metric_name = "read_transaction", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" },

	{ .group_name = "performance:iommu:afu0", .metric_name = "write_transaction", .data_type = FPGA_METRIC_DATATYPE_INT, .metric_units = "" }

};

#endif //__FPGA_METRICS_METADATA_H__
// Copyright(c) 2021, Intel Corporation
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

#include <endian.h>
#include <limits.h>
#include <time.h>
#include <opae/fpga.h>

#include "board_event_log.h"

#define BEL_BLOCK_SIZE     0x1000
#define BEL_BLOCK_COUNT    63
#define BEL_PTR_OFFSET     (BEL_BLOCK_COUNT * BEL_BLOCK_SIZE)
#define BEL_PTR_SIZE       4
#define BEL_LABEL_FMT      "%-*s : "

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(*a))

enum bel_magic {
	BEL_POWER_ON_STATUS  = 0x53696C12,
	BEL_POWER_OFF_STATUS = 0x53696C34,
	BEL_SENSORS_STATE    = 0x53696C56,
	BEL_SENSORS_STATUS   = 0x53696C78,
	BEL_PCI_ERROR_STATUS = 0x53696C9A,
};

enum bel_power_regulator {
	BEL_PWR_REG_IR38062_VOUT = 0,
	BEL_PWR_REG_IR38062_IOUT,
	BEL_PWR_REG_IR38062_VIN,
	BEL_PWR_REG_IR38062_TEMP,
	BEL_PWR_REG_IR38063_VOUT,
	BEL_PWR_REG_IR38063_IOUT,
	BEL_PWR_REG_IR38063_VIN,
	BEL_PWR_REG_IR38063_TEMP,
	BEL_PWR_REG_ISL68220_VOUT,
	BEL_PWR_REG_ISL68220_IOUT,
	BEL_PWR_REG_ISL68220_VIN,
	BEL_PWR_REG_ISL68220_TEMP,
};

struct bel_sensor_info {
	uint32_t id;
	const char *label;
	const char *unit;
	uint32_t resolution;
};

static struct bel_sensor_info bel_sensor_info[] = {
	{ .id = 1, .label = "FPGA E-TILE Max Temperature", .unit = "°C", .resolution = 2 },
	{ .id = 2, .label = "FPGA E-TILE Temperature#1", .unit = "°C", .resolution = 2 },
	{ .id = 3, .label = "FPGA E-TILE Temperature#2", .unit = "°C", .resolution = 2 },
	{ .id = 4, .label = "FPGA E-TILE Temperature#3", .unit = "°C", .resolution = 2 },
	{ .id = 5, .label = "FPGA E-TILE Temperature#4", .unit = "°C", .resolution = 2 },
	{ .id = 6, .label = "FPGA P-TILE Temperature", .unit = "°C", .resolution = 2 },
	{ .id = 7, .label = "FPGA FABRIC Max Temperature", .unit = "°C", .resolution = 2 },
	{ .id = 8, .label = "FPGA FABRIC DTS#1", .unit = "°C", .resolution = 2 },
	{ .id = 9, .label = "FPGA FABRIC DTS#2", .unit = "°C", .resolution = 2 },
	{ .id = 10, .label = "FPGA FABRIC DTS#3", .unit = "°C", .resolution = 2 },
	{ .id = 11, .label = "FPGA FABRIC DTS#4", .unit = "°C", .resolution = 2 },
	{ .id = 12, .label = "FPGA FABRIC DTS#5", .unit = "°C", .resolution = 2 },
	{ .id = 13, .label = "FPGA FABRIC RDTS#1", .unit = "°C", .resolution = 2 },
	{ .id = 14, .label = "FPGA FABRIC RDTS#2", .unit = "°C", .resolution = 2 },
	{ .id = 15, .label = "FPGA FABRIC RDTS#3", .unit = "°C", .resolution = 2 },
	{ .id = 16, .label = "FPGA FABRIC RDTS#4", .unit = "°C", .resolution = 2 },
	{ .id = 60, .label = "Board Top Near FPGA", .unit = "°C", .resolution = 2 },
	{ .id = 61, .label = "Board Bottom Near CVL", .unit = "°C", .resolution = 2 },
	{ .id = 62, .label = "Board Top East Near VRs Temperature", .unit = "°C", .resolution = 2 },
	{ .id = 63, .label = "Columbiaville Die Temperature", .unit = "°C", .resolution = 2 },
	{ .id = 64, .label = "Board Rear Side Temperature", .unit = "°C", .resolution = 2 },
	{ .id = 65, .label = "Board Front Side Temperature", .unit = "°C", .resolution = 2 },
	{ .id = 66, .label = "QSFP1(Primary) Case Temperature", .unit = "°C", .resolution = 2 },
	{ .id = 67, .label = "QSFP2(Secondary) Case Temperature", .unit = "°C", .resolution = 2 },
	{ .id = 68, .label = "FPGA Core Voltage Phase 0 VR Temperature", .unit = "°C", .resolution = 2 },
	{ .id = 69, .label = "FPGA Core Voltage Phase 1 VR Temperature", .unit = "°C", .resolution = 2 },
	{ .id = 70, .label = "FPGA Core Voltage Phase 2 VR Temperature", .unit = "°C", .resolution = 2 },
	{ .id = 71, .label = "FPGA Core Voltage VR Controller Temperature", .unit = "°C", .resolution = 2 },
	{ .id = 72, .label = "FPGA VCCH VR Temperature", .unit = "°C", .resolution = 2 },
	{ .id = 73, .label = "FPGA VCC_1V2 VR Temperature", .unit = "°C", .resolution = 2 },
	{ .id = 74, .label = "FPGA VCCH & VCC_1V2 VR Controller Temperature", .unit = "°C", .resolution = 2 },
	{ .id = 75, .label = "3V3 VR Temperature", .unit = "°C", .resolution = 2 },
	{ .id = 76, .label = "CVL Core Voltage VR Temperature", .unit = "°C", .resolution = 2 },
	{ .id = 77, .label = "FPGA P-Tile Temperature [Remote]", .unit = "°C", .resolution = 2 },
	{ .id = 78, .label = "FPGA E-Tile Temperature [Remote]", .unit = "°C", .resolution = 2 },
	{ .id = 79, .label = "FPGA Core Temperature [Remote]", .unit = "°C", .resolution = 2 },
	{ .id = 80, .label = "FPGA Corner Temperature [Remote]", .unit = "°C", .resolution = 2 },
	{ .id = 100, .label = "Inlet 12V PCIe Rail [Voltage]", .unit = "mV", .resolution = 1 },
	{ .id = 101, .label = "Inlet 12V PCIe Rail [Current]", .unit = "mA", .resolution = 1 },
	{ .id = 102, .label = "Inlet 12V Aux Rail [Voltage]", .unit = "mV", .resolution = 1 },
	{ .id = 103, .label = "Inlet 12V Aux Rail [Current]", .unit = "mA", .resolution = 1 },
	{ .id = 104, .label = "Inlet 3V3 PCIe Rail [Voltage]", .unit = "mV", .resolution = 1 },
	{ .id = 105, .label = "Inlet 3V3 PCIe Rail [Current]", .unit = "mA", .resolution = 1 },
	{ .id = 108, .label = "Board Power", .unit = "mW", .resolution = 1 },
	{ .id = 130, .label = "FPGA Core Voltage Rail [Voltage]", .unit = "mV", .resolution = 1 },
	{ .id = 131, .label = "FPGA Core Voltage Rail [Current]", .unit = "mA", .resolution = 1 },
	{ .id = 132, .label = "FPGA VCCH Rail [Voltage]", .unit = "mV", .resolution = 1 },
	{ .id = 133, .label = "FPGA VCCH Rail [Current]", .unit = "mA", .resolution = 1 },
	{ .id = 134, .label = "FPGA VCC_1V2 Rail [Voltage]", .unit = "mV", .resolution = 1 },
	{ .id = 135, .label = "FPGA VCC_1V2 Rail [Current]", .unit = "mA", .resolution = 1 },
	{ .id = 136, .label = "FPGA VCCH_GXER_1V1 & VCCA_1V8 [Voltage]", .unit = "mV", .resolution = 1 },
	{ .id = 137, .label = "FPGA VCCH_GXER_1V1 & VCCA_1V8 [Current]", .unit = "mA", .resolution = 1 },
	{ .id = 138, .label = "FPGA VCCIO_1V2 [Voltage]", .unit = "mV", .resolution = 1 },
	{ .id = 139, .label = "FPGA VCCIO_1V2 [Current]", .unit = "mA", .resolution = 1 },
	{ .id = 140, .label = "CVL Non Core Rails Inlet [Voltage]", .unit = "mV", .resolution = 1 },
	{ .id = 141, .label = "CVL Non Core Rails Inlet [Current]", .unit = "mA", .resolution = 1 },
	{ .id = 142, .label = "MAX10 & Board CLK PWR 3V3 Inlet [Voltage]", .unit = "mV", .resolution = 1 },
	{ .id = 143, .label = "MAX10 & Board CLK PWR 3V3 Inlet [Current]", .unit = "mA", .resolution = 1 },
	{ .id = 144, .label = "CVL Core Voltage Rail [Voltage]", .unit = "mV", .resolution = 1 },
	{ .id = 145, .label = "CVL Core Voltage Rail [Current]", .unit = "mA", .resolution = 1 },
	{ .id = 148, .label = "Board 3V3 VR [Voltage]", .unit = "mV", .resolution = 1 },
	{ .id = 149, .label = "Board 3V3 VR [Current]", .unit = "mA", .resolution = 1 },
	{ .id = 150, .label = "QSFP 3V3 Rail [Voltage]", .unit = "mV", .resolution = 1 },
	{ .id = 151, .label = "QSFP 3V3 Rail [Current]", .unit = "mA", .resolution = 1 },
	{ .id = 152, .label = "QSFP (Primary) Supply Voltage Rail [Voltage]", .unit = "mV", .resolution = 1 },
	{ .id = 153, .label = "QSFP (Secondary) Supply Voltage Rail [Voltage]", .unit = "mV", .resolution = 1 },
	{ .id = 180, .label = "VCCCLK_GXER_2V5 Voltage", .unit = "mV", .resolution = 1 },
	{ .id = 181, .label = "AVDDH_1V1_CVL Voltage", .unit = "mV", .resolution = 1 },
	{ .id = 182, .label = "VDDH_1V8_CVL Voltage", .unit = "mV", .resolution = 1 },
	{ .id = 183, .label = "VCCA_PLL Voltage", .unit = "mV", .resolution = 1 },
	{ .id = 184, .label = "VCCRT_GXER_0V9 Voltage", .unit = "mV", .resolution = 1 },
	{ .id = 185, .label = "VCCRT_GXPL_0V9 Voltage", .unit = "mV", .resolution = 1 },
	{ .id = 186, .label = "VCCH_GXPL_1V8 Voltage", .unit = "mV", .resolution = 1 },
	{ .id = 187, .label = "VCCPT_1V8 Voltage", .unit = "mV", .resolution = 1 },
	{ .id = 188, .label = "VCC_3V3_M10  Voltage", .unit = "mV", .resolution = 1 },
	{ .id = 189, .label = "VCC_1V8_M10  Voltage", .unit = "mV", .resolution = 1 },
	{ .id = 190, .label = "VCC_1V2_EMIF1_2_3 Voltage", .unit = "mV", .resolution = 1 },
	{ .id = 191, .label = "VCC_1V2_EMIF4_5 Voltage", .unit = "mV", .resolution = 1 },
	{ .id = 192, .label = "VCCA_1V8  Voltage", .unit = "mV", .resolution = 1 },
	{ .id = 193, .label = "VCCH_GXER_1V1 Voltage", .unit = "mV", .resolution = 1 },
	{ .id = 194, .label = "AVDD_ETH_0V9_CVL  Voltage", .unit = "mV", .resolution = 1 },
	{ .id = 195, .label = "AVDD_PCIE_0V9_CVL Voltage", .unit = "mV", .resolution = 1 },
	{ .id = 32768, .label = "Virt FPGA Temperature", .unit = "°C", .resolution = 2 },
};

static struct bel_sensor_info bel_power_regulator_info[] = {
	[BEL_PWR_REG_IR38062_VOUT]  = { .label = "IR38062 Voltage",      .unit = "mV", .resolution = 1 },
	[BEL_PWR_REG_IR38062_IOUT]  = { .label = "IR38062 Current",      .unit = "mA", .resolution = 1 },
	[BEL_PWR_REG_IR38062_VIN]   = { .label = "IR38062 Temperature",  .unit = "°C", .resolution = 1 },
	[BEL_PWR_REG_IR38062_TEMP]  = { .label = "IR38062 Input",        .unit = "mV", .resolution = 1 },
	[BEL_PWR_REG_IR38063_VOUT]  = { .label = "IR38063 Voltage",      .unit = "mV", .resolution = 1 },
	[BEL_PWR_REG_IR38063_IOUT]  = { .label = "IR38063 Current",      .unit = "mA", .resolution = 1 },
	[BEL_PWR_REG_IR38063_VIN]   = { .label = "IR38063 Temperature",  .unit = "°C", .resolution = 1 },
	[BEL_PWR_REG_IR38063_TEMP]  = { .label = "IR38063 Input",        .unit = "mV", .resolution = 1 },
	[BEL_PWR_REG_ISL68220_VOUT] = { .label = "ISL68220 Voltage",     .unit = "mV", .resolution = 1 },
	[BEL_PWR_REG_ISL68220_IOUT] = { .label = "ISL68220 Current",     .unit = "mA", .resolution = 1 },
	[BEL_PWR_REG_ISL68220_VIN]  = { .label = "ISL68220 Temperature", .unit = "°C", .resolution = 1 },
	[BEL_PWR_REG_ISL68220_TEMP] = { .label = "ISL68220 Input",       .unit = "mV", .resolution = 1 },
};

static void bel_print_bool(const char *label, uint32_t value, size_t offset, const char *one, const char *zero)
{
	bool bit = (value >> offset) & 0x1;

	printf("      " BEL_LABEL_FMT "%s\n", 46, label, bit ? one : zero);
}

static void bel_print_bit(const char *label, uint32_t value, size_t offset)
{
	bel_print_bool(label, value, offset, "1", "0");
}

static void bel_print_pass(const char *label, uint32_t value, size_t offset)
{
	bel_print_bool(label, value, offset, "No", "Yes");
}

static void bel_print_fail(const char *label, uint32_t value, size_t offset)
{
	bel_print_bool(label, value, offset, "Yes", "No");
}

static void bel_print_field(const char *label, uint32_t value, size_t first, size_t last)
{
	uint32_t mask = UINT_MAX >> (32 - (last - first));
	uint32_t field = (value >> first) & mask;

	printf("      " BEL_LABEL_FMT "0x%x\n", 46, label, field);
}

static void bel_print_value(const char *label, uint32_t value)
{
	printf("    " BEL_LABEL_FMT "0x%08x\n", 48, label, value);
}

static void bel_print_header(const char *label, struct bel_header *header)
{
	time_t time_sec = ((uint64_t)header->timespamp_high << 32) | header->timestamp_low;
	char time_str[26];

	if (ctime_r(&time_sec, time_str) == NULL) {
		OPAE_ERR("Failed to format time: %s", strerror(errno));
		return;
	}

	printf("  " BEL_LABEL_FMT "%s", 50, label, time_str);
}

static void bel_print_power_on_status(struct bel_power_on_status *status)
{
	if (status->header.magic != BEL_POWER_ON_STATUS)
		return;

	bel_print_header("Power On Status Time", &status->header);

	/* Register 0x80 */
	bel_print_value("Status", status->status);
	bel_print_field("Power On Code FPGA", status->status, 24, 28);
	bel_print_field("Power On Code CVL", status->status, 28, 32);

	/* Register 0xa0 */
	bel_print_value("FPGA_Status", status->fpga_status);
	bel_print_field("FPGA Page", status->fpga_status, 0, 3);
	bel_print_bit("FPGA Configued Page", status->fpga_status, 3);
	bel_print_bit("FPGA Config Timeline", status->fpga_status, 4);
	bel_print_bit("Flow Done", status->fpga_status, 5);
	bel_print_field("FSM State", status->fpga_status, 12, 16);

	/* Register 0x */
	bel_print_value("FPGA_Config Status", status->fpga_config_status);
	bel_print_field("Config Status", status->fpga_config_status, 0, 4);
	bel_print_bit("Config FW Seq Fail", status->fpga_config_status, 4);
	bel_print_field("nStatus Stuck Low Restarts", status->fpga_config_status, 5, 8);
	bel_print_field("Image Fail Recycle Count", status->fpga_config_status, 8, 10);
	bel_print_field("1st Power On Page number", status->fpga_config_status, 10, 13);
	bel_print_field("2nd Power On Page number", status->fpga_config_status, 13, 16);
	bel_print_field("3nd Power On Page number", status->fpga_config_status, 16, 19);
}

static void bel_print_sensor_alert(uint32_t sensor_alert, size_t offset)
{
	struct bel_sensor_info *info = &bel_sensor_info[offset];
	size_t last = sizeof(sensor_alert) * 8;
	size_t i;

	if (offset + last > ARRAY_SIZE(bel_sensor_info))
		last = ARRAY_SIZE(bel_sensor_info) - offset;

	for (i = 0; i < last; i++, info++)
		bel_print_fail(info->label, sensor_alert, i);
}

static void bel_print_power_off_status(struct bel_power_off_status *status, bool print_bits)
{
	if (status->header.magic != BEL_POWER_OFF_STATUS)
		return;

	bel_print_header("Power Off Status Time", &status->header);

	/* Register 0x98 */
	bel_print_value("Power Good Status", status->power_good_status);

	if (print_bits) {
		bel_print_bit("pg_12v_aux_efuse",     status->power_good_status,  0);
		bel_print_bit("pg_12v_pcie_efuse",    status->power_good_status,  1);
		bel_print_bit("pg_vcc_5v",            status->power_good_status,  2);
		bel_print_bit("pg_vcc_3v3",           status->power_good_status,  3);
		bel_print_bit("pg_vccl_fpga_vid",     status->power_good_status,  4);
		bel_print_bit("pg_vccl_sdm_0v8",      status->power_good_status,  5);
		bel_print_bit("pg_vcch_0v9",          status->power_good_status,  6);
		bel_print_bit("pg_vcch_gxer_1v1",     status->power_good_status,  7);
		bel_print_bit("pg_vcca_1v8",          status->power_good_status,  8);
		bel_print_bit("pg_vccclk_gxer_2v5",   status->power_good_status,  9);
		bel_print_bit("pg_vpp_2v5",           status->power_good_status, 10);
		bel_print_bit("pg_vccio_1v8",         status->power_good_status, 11);
		bel_print_bit("pg_sdm_1v8",           status->power_good_status, 12);
		bel_print_bit("pg_vcc_1v2",           status->power_good_status, 13);
		bel_print_bit("pg_vtt_0v6",           status->power_good_status, 14);
		bel_print_bit("pg_vcc_1v1_cvl",       status->power_good_status, 15);
		bel_print_bit("pg_3v3_1v8_cvl",       status->power_good_status, 16);
		bel_print_bit("pg_vdd_0v8_cvl",       status->power_good_status, 17);
		bel_print_bit("pg_avdd_eth_0v9_cvl",  status->power_good_status, 18);
		bel_print_bit("pg_avdd_pcie_0v9_cvl", status->power_good_status, 19);
		bel_print_bit("pg_avddh_1v1_cvl",     status->power_good_status, 20);
		bel_print_bit("pg_pwr_qsfp0n",        status->power_good_status, 21);
		bel_print_bit("pg_pwr_qsfp1n",        status->power_good_status, 22);
		bel_print_bit("FPGA_THERM_SHDN",      status->power_good_status, 23);
		bel_print_bit("EDGE_PWR_SHDN",        status->power_good_status, 24);
		bel_print_bit("FPGA_NCATTRIP",        status->power_good_status, 25);
		bel_print_bit("VCC_12V_AUX_UV",       status->power_good_status, 26);
		bel_print_bit("VCC_12V_PCIE_UV",      status->power_good_status, 27);
		bel_print_bit("VCC_3V3_PCIE_UV",      status->power_good_status, 28);
		bel_print_bit("VCC_12V_3V3_IN_OV",    status->power_good_status, 29);
		bel_print_bit("QSFPA_MODPRES",        status->power_good_status, 30);
		bel_print_bit("QSFPB_MODPRES",        status->power_good_status, 31);
	}

	/* Register 0x84 */
	bel_print_value("Power Good Record 1", status->record_1);

	if (print_bits) {
		bel_print_bit("pg_12v_aux_efuse",     status->record_1,  0);
		bel_print_bit("pg_12v_pcie_efuse",    status->record_1,  1);
		bel_print_bit("pg_vcc_5v",            status->record_1,  2);
		bel_print_bit("pg_vcc_3v3",           status->record_1,  3);
		bel_print_bit("pg_vccl_fpga_vid",     status->record_1,  4);
		bel_print_bit("pg_vccl_sdm_0v8",      status->record_1,  5);
		bel_print_bit("pg_vcch_0v9",          status->record_1,  6);
		bel_print_bit("pg_vcch_gxer_1v1",     status->record_1,  7);
		bel_print_bit("pg_vcca_1v8",          status->record_1,  8);
		bel_print_bit("pg_vccclk_gxer_2v5",   status->record_1,  9);
		bel_print_bit("pg_vpp_2v5",           status->record_1, 10);
		bel_print_bit("pg_vccio_1v8",         status->record_1, 11);
		bel_print_bit("pg_sdm_1v8",           status->record_1, 12);
		bel_print_bit("pg_vcc_1v2",           status->record_1, 13);
		bel_print_bit("pg_vtt_0v6",           status->record_1, 14);
		bel_print_bit("pg_vcc_1v1_cvl",       status->record_1, 15);
		bel_print_bit("pg_3v3_1v8_cvl",       status->record_1, 16);
		bel_print_bit("pg_vdd_0v8_cvl",       status->record_1, 17);
		bel_print_bit("pg_avdd_eth_0v9_cvl",  status->record_1, 18);
		bel_print_bit("pg_avdd_pcie_0v9_cvl", status->record_1, 19);
		bel_print_bit("pg_avddh_1v1_cvl",     status->record_1, 20);
		bel_print_bit("pg_pwr_qsfp0n",        status->record_1, 21);
		bel_print_bit("pg_pwr_qsfp1n",        status->record_1, 22);
	}

	/* Register 0x88 */
	bel_print_value("Power Good Record 2", status->record_2);

	if (print_bits) {
		bel_print_bit("VCC_12V_AUX_UV",    status->record_2,  0);
		bel_print_bit("VCC_12V_PCIE_UV",   status->record_2,  1);
		bel_print_bit("VCC_3V3_PCIE_UV",   status->record_2,  2);
		bel_print_bit("VCC_12V_3V3_IN_OV", status->record_2,  3);
		bel_print_bit("FPGA_THERM_SHDN",   status->record_2,  4);
		bel_print_bit("EDGE_PWR_SHDN",     status->record_2,  5);
		bel_print_bit("FPGA_NCATTRIP",     status->record_2,  6);
		/* reserved bit 7 */
		bel_print_bit("PM_ALERTN_3V3",     status->record_2,  8);
		bel_print_bit("FPGA_VID_ALERTN",   status->record_2,  9);
		bel_print_bit("VR_VID_ALERTN",     status->record_2, 10);
		bel_print_bit("FPGA_THERM_ALERTN", status->record_2, 11);
		bel_print_bit("CVL_THERM_ALERTN",  status->record_2, 12);
		bel_print_bit("EDGE_PWR_WARN",     status->record_2, 13);
		bel_print_bit("SI5392_LOL",        status->record_2, 14);
	}

	/* Register 0x90 */
	bel_print_value("Sequencer Status 1", status->sequencer_status_1);

	if (print_bits) {
		bel_print_bit("IDLE_ST",                 status->sequencer_status_1,  0);
		bel_print_bit("PWR_CLS_DEC_FPGA_ST",     status->sequencer_status_1,  1);
		bel_print_bit("PG_3V3_5V_ST",            status->sequencer_status_1,  2);
		bel_print_bit("WAIT_10MS_FPGA_ST",       status->sequencer_status_1,  3);
		bel_print_bit("FPGA_GRP1_EN_ST",         status->sequencer_status_1,  4);
		bel_print_bit("FPGA_GRP2_EN_ST",         status->sequencer_status_1,  5);
		bel_print_bit("FPGA_GRP3_EN_ST",         status->sequencer_status_1,  6);
		bel_print_bit("PG_VTT_0V6_CHK_ST",       status->sequencer_status_1,  7);
		bel_print_bit("FPGA_GRP1_PWR_DWN_ST",    status->sequencer_status_1,  8);
		bel_print_bit("FPGA_GRP2_PWR_DWN_ST",    status->sequencer_status_1,  9);
		bel_print_bit("FPGA_GRP3_PWR_DWN_ST",    status->sequencer_status_1, 10);
		bel_print_bit("FPGA_PWR_ON_ST",          status->sequencer_status_1, 11);
		bel_print_bit("FPGA_PWR_OFF_ST",         status->sequencer_status_1, 12);
		bel_print_bit("PWR_CLS_DEC_CVL_ST",      status->sequencer_status_1, 16);
		bel_print_bit("CVL_VCC_1V1_ST",          status->sequencer_status_1, 17);
		bel_print_bit("CVL_WAIT_100US_ST",       status->sequencer_status_1, 18);
		bel_print_bit("CVL_3V3_1V8_EN_ST",       status->sequencer_status_1, 19);
		bel_print_bit("CVL_VDD_0V8_EN_ST",       status->sequencer_status_1, 20);
		bel_print_bit("CVL_AVDD_ETH_EN_ST",      status->sequencer_status_1, 21);
		bel_print_bit("CVL_AVDD_PCIE_1V1_EN_ST", status->sequencer_status_1, 22);
		bel_print_bit("CVL_SI5392_CHK_ST",       status->sequencer_status_1, 23);
		bel_print_bit("CVL_LAN_100US_WAIT_ST",   status->sequencer_status_1, 24);
		bel_print_bit("CVL_LAN_PG_ST",           status->sequencer_status_1, 25);
		bel_print_bit("CVL_PWR_ON_ST",           status->sequencer_status_1, 26);
		bel_print_bit("CVL_LAN_PWR_DWN_ST",      status->sequencer_status_1, 27);
		bel_print_bit("CVL_AVDD_PWR_DWN_ST",     status->sequencer_status_1, 28);
		bel_print_bit("CVL_VDD_0V8_PWR_DWN_ST",  status->sequencer_status_1, 29);
		bel_print_bit("CVL_3V3_1V8_PWR_DWN_ST",  status->sequencer_status_1, 30);
		bel_print_bit("CVL_PWR_OFF_ST",          status->sequencer_status_1, 31);
	}

	/* Register 0x94 */
	bel_print_value("Sequencer Status 2", status->sequencer_status_2);

	if (print_bits) {
		bel_print_bit("EN_VCCL_FPGA_VID",         status->sequencer_status_2,  0);
		bel_print_bit("EN_VCCL_SDM_0V8_VCCH_0V9", status->sequencer_status_2,  1);
		bel_print_bit("EN_FPGA_GRP2",             status->sequencer_status_2,  2);
		bel_print_bit("EN_VPP_2V5",               status->sequencer_status_2,  3);
		bel_print_bit("EN_VCCIO_1V8_SDM_1V8",     status->sequencer_status_2,  4);
		bel_print_bit("EN_VCC_1V2",               status->sequencer_status_2,  5);
		bel_print_bit("EN_3V3_CVL",               status->sequencer_status_2,  6);
		bel_print_bit("EN_1V8_CVL",               status->sequencer_status_2,  7);
		bel_print_bit("EN_VDD_0V8_CVL",           status->sequencer_status_2,  8);
		bel_print_bit("EN_AVDD_ETH_0V9_CVL",      status->sequencer_status_2,  9);
		bel_print_bit("EN_AVDD_PCIE_0V9_CVL",     status->sequencer_status_2, 10);
		bel_print_bit("EN_AVDDH_1V1_CVL",         status->sequencer_status_2, 11);
		bel_print_bit("EN_PWR_QSFP0",             status->sequencer_status_2, 12);
		bel_print_bit("EN_PWR_QSFP1",             status->sequencer_status_2, 13);
		/* reserved bit 14 and 15 */
		bel_print_bit("SI5392_LOL",               status->sequencer_status_2, 16);
		bel_print_bit("POWER_GOOD",               status->sequencer_status_2, 17);
		bel_print_bit("LAN_PWR_GOOD",             status->sequencer_status_2, 18);
		bel_print_bit("PM_ALERTN_3V3",            status->sequencer_status_2, 19);
		bel_print_bit("FPGA_VID_ALERTN",          status->sequencer_status_2, 20);
		bel_print_bit("VR_VID_ALERTN",            status->sequencer_status_2, 21);
		bel_print_bit("FPGA_THERM_ALERTN",        status->sequencer_status_2, 22);
		bel_print_bit("CVL_THERM_ALERTN",         status->sequencer_status_2, 23);
		bel_print_bit("EDGE_PWR_WARN",            status->sequencer_status_2, 24);
	}

	/* Register 0x50 */
	bel_print_value("GPI Status", status->general_purpose_input_status);

	if (print_bits) {
		bel_print_bit("Max10 SEU error",      status->general_purpose_input_status,  0);
		bel_print_bit("FPGA SEU error",       status->general_purpose_input_status,  1);
		bel_print_bit("QSFPA_MODPRES",        status->general_purpose_input_status,  2);
		bel_print_bit("QSFPB_MODPRES",        status->general_purpose_input_status,  3);
		bel_print_bit("PCIE_CLK_LOS",         status->general_purpose_input_status,  4);
		bel_print_bit("PTP_CLK_LOL",          status->general_purpose_input_status,  5);
		bel_print_bit("Si5392_LOL",           status->general_purpose_input_status,  6);
		bel_print_bit("FPGA_nCATTrip",        status->general_purpose_input_status,  7);
		bel_print_bit("STRAP_0_PCLASS0",      status->general_purpose_input_status,  8);
		bel_print_bit("STRAP_1_PCLASS1",      status->general_purpose_input_status,  9);
		bel_print_bit("SMBus_I2C_ADDR",       status->general_purpose_input_status, 10);
		bel_print_bit("STRAP_3_CVL_PRSNT0_n", status->general_purpose_input_status, 11);
		bel_print_bit("FLT_CFP_ISL",          status->general_purpose_input_status, 12);
		bel_print_bit("IO_EXPNDR_INT_n",      status->general_purpose_input_status, 13);
	}

	/* Register 0x410 */
	bel_print_value("Sensor Failed", status->sensor_failed);

	if (print_bits) {
		bel_print_pass("fpga_remote_temp",       status->sensor_failed,  0);
		bel_print_pass("board_temp",             status->sensor_failed,  1);
		bel_print_pass("inlet_12v_pcie",         status->sensor_failed,  2);
		bel_print_pass("fpga_vcch_gxer",         status->sensor_failed,  3);
		bel_print_pass("max10_board_clk_pwr",    status->sensor_failed,  4);
		bel_print_pass("cvl_core_vol_temp",      status->sensor_failed,  5);
		bel_print_pass("board_3v3_vol_temp",     status->sensor_failed,  6);
		bel_print_pass("fpga_vcch",              status->sensor_failed,  7);
		bel_print_pass("fpga_core_vol_pwr_temp", status->sensor_failed,  8);
		bel_print_pass("fpga_fab_tile_temp",     status->sensor_failed,  9);
		bel_print_pass("overall_devices",        status->sensor_failed, 31);
	}

	/* Register 0x414 */
	bel_print_value("Sensor Alert 1", status->sensor_alert_1);

	if (print_bits)
		bel_print_sensor_alert(status->sensor_alert_1, 0);

	/* Register 0x418 */
	bel_print_value("Sensor Alert 2", status->sensor_alert_2);

	if (print_bits)
		bel_print_sensor_alert(status->sensor_alert_1, 32);

	/* Register 0x41c */
	bel_print_value("Sensor Alert 3", status->sensor_alert_3);

	if (print_bits)
		bel_print_sensor_alert(status->sensor_alert_1, 64);
}

static size_t bel_print_sensor(struct bel_sensor_state *state, size_t last)
{
	struct bel_sensor_info *info;
	size_t next = last + 1;

	/* Search the info array starting from one past the previous printed sensor */
	while (next != last) {
		info = &bel_sensor_info[next];

		if (info->id == state->id)
			break;

		next = (next + 1) % ARRAY_SIZE(bel_sensor_info);
	}

	/* Print nothing if sensor wasn't found */
	if (next == last)
		return last;

	printf("    " BEL_LABEL_FMT, 48, info->label);
	if (state->reading != INT_MAX)
		printf("%6u %s\n", state->reading / info->resolution, info->unit);
	else
		printf("%9s\n", "N/A");

	return next;
}

static void bel_print_sensors_state(struct bel_sensors_state *state)
{
	size_t idx = -1;
	size_t i;

	if (state->header.magic != BEL_SENSORS_STATE)
		return;

	bel_print_header("Sensor State Time", &state->header);

	for (i = 0; i < ARRAY_SIZE(state->sensor_state); i++)
		idx = bel_print_sensor(&state->sensor_state[i], idx);
}

static void bel_print_sensors_status_ext(const char *label, struct bel_ext_status *status, size_t idx)
{
	struct bel_sensor_info *info = &bel_power_regulator_info[idx];
	char l[32];
	size_t i;

	snprintf(l, sizeof(l), "%s Status Word", label);
	bel_print_value(l, status->word);
	bel_print_bit("None/Unkown", status->word, 0);
	bel_print_bit("CML", status->word, 1);
	bel_print_bit("Temperature", status->word, 2);
	bel_print_bit("Vin Undervoltage", status->word, 3);
	bel_print_bit("Iout Overcurrent", status->word, 4);
	bel_print_bit("Vout Overvoltage", status->word, 5);
	bel_print_bit("Off", status->word, 6);
	bel_print_bit("Busy", status->word, 7);
	bel_print_bit("None/Unknown", status->word, 8);
	bel_print_bit("Other", status->word, 9);
	bel_print_bit("Fans", status->word, 10);
	bel_print_bit("Power Good", status->word, 11);
	bel_print_bit("Manufacturer Specific Fault", status->word, 12);
	bel_print_bit("Input", status->word, 13);
	bel_print_bit("Iout/Pout", status->word, 14);
	bel_print_bit("Vout", status->word, 15);

	for (i = 0; i < 4; i++) {
		printf("    " BEL_LABEL_FMT "%7u %s\n", 48, info->label, status->data[i + 1], info->unit);
		info++;
	}

	snprintf(l, sizeof(l), "%s Status CML", label);
	bel_print_value(l, status->cml);
	bel_print_bit("Other Fault", status->cml, 0);
	bel_print_bit("Communication Fault", status->cml, 1);
	bel_print_bit("Processor Fault", status->cml, 3);
	bel_print_bit("Memory Fault", status->cml, 4);
	bel_print_bit("Packet Error Check Fault", status->cml, 5);
	bel_print_bit("Invalid/Unsupported Data", status->cml, 6);
	bel_print_bit("Invalid/Unsupported Command", status->cml, 7);
}

static void bel_print_sensors_status(struct bel_sensors_status *status)
{
	if (status->header.magic != BEL_SENSORS_STATUS)
		return;

	bel_print_header("Sensor Status Time", &status->header);

	bel_print_value("INA3221 1 Mask Enable", status->ina3221_1_mask_enable);
	bel_print_value("INA3221 2 Mask Enable", status->ina3221_2_mask_enable);
	bel_print_value("INA3221 3 Mask Enable", status->ina3221_3_mask_enable);
	bel_print_sensors_status_ext("IR38062", &status->ir38062, BEL_PWR_REG_IR38062_VOUT);
	bel_print_sensors_status_ext("IR38063", &status->ir38063, BEL_PWR_REG_IR38063_VOUT);
	bel_print_sensors_status_ext("ISL68220", &status->isl68220, BEL_PWR_REG_ISL68220_VOUT);
	bel_print_value("ED8401 Status", status->ed8401_status);
}

static void bel_print_pci_error_status(struct bel_pci_error_status *status)
{
	if (status->header.magic != BEL_PCI_ERROR_STATUS)
		return;

	bel_print_header("PCI Error Status Time", &status->header);

}

uint32_t bel_ptr_count(void)
{
	return BEL_BLOCK_COUNT;
}

uint32_t bel_ptr_next(uint32_t ptr)
{
	if (ptr == 0)
		return BEL_BLOCK_COUNT - 1;
	else
		return ptr - 1;
}

fpga_result bel_ptr(fpga_object fpga_object, uint32_t *ptr)
{
	fpga_result res;
	uint32_t data;

	res = fpgaObjectRead(fpga_object, (uint8_t *)&data, BEL_PTR_OFFSET,
				sizeof(data), FPGA_OBJECT_RAW);

	if (res != FPGA_OK)
		return res;

	if (data == UINT_MAX)
		data = 0;

	if (ptr)
		*ptr = le32toh(data);

	return res;
}

fpga_result bel_read(fpga_object fpga_object, uint32_t ptr, struct bel_event *event)
{
	size_t offset = ptr * BEL_BLOCK_SIZE;
	size_t count = sizeof(*event) / sizeof(uint32_t);
	fpga_result res;

	if (ptr >= BEL_BLOCK_COUNT)
		return FPGA_INVALID_PARAM;

	res = fpgaObjectRead(fpga_object, (uint8_t *)event, offset, sizeof(*event), FPGA_OBJECT_RAW);
	if (res != FPGA_OK)
		return res;

	while (--count)
		event->data[count] = le32toh(event->data[count]);

	return res;
}

void bel_print(struct bel_event *event, bool print_sensors, bool print_bits)
{
	bel_print_power_on_status(&event->power_on_status);
	bel_print_power_off_status(&event->power_off_status, print_bits);

	if (print_sensors) {
		bel_print_sensors_state(&event->sensors_state);
		bel_print_sensors_status(&event->sensors_status);
	}

	bel_print_pci_error_status(&event->pci_error_status);
}

void bel_timespan(struct bel_event *event, uint32_t idx)
{
	struct bel_header *header_off = &event->power_off_status.header;
	struct bel_header *header_on = &event->power_on_status.header;
	time_t off_sec = ((uint64_t)header_off->timespamp_high << 32) | header_off->timestamp_low;
	time_t on_sec = ((uint64_t)header_on->timespamp_high << 32) | header_on->timestamp_low;
	char off_str[26] = { '\0' };
	char on_str[26] = { '\0' };

	if (header_on->magic != BEL_POWER_ON_STATUS)
		return;

	if (ctime_r(&on_sec, on_str) == NULL) {
		OPAE_ERR("Failed to format time: %s", strerror(errno));
		return;
	}

	on_str[24] = '\0';

	if (header_off->magic == BEL_POWER_OFF_STATUS) {
		if (ctime_r(&off_sec, off_str) == NULL) {
			OPAE_ERR("Failed to format time: %s", strerror(errno));
			return;
		}

		off_str[24] = '\0';
	}

	printf("Boot %u: %s — %s\n", idx, on_str, off_str);
}

bool bel_empty(struct bel_event *event)
{
	return event->power_on_status.header.magic == UINT_MAX;
}

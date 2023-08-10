// Copyright(c) 2021-2022, Intel Corporation
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
#include <ofs/ofs_defs.h>
#include <opae/fpga.h>

#include "board_event_log.h"

#define BEL_BLOCK_SIZE     0x1000
#define BEL_BLOCK_COUNT    63
#define BEL_PTR_OFFSET     (BEL_BLOCK_COUNT * BEL_BLOCK_SIZE)
#define BEL_PTR_SIZE       4
#define BEL_LABEL_FMT      "%-*s : "

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(*a))

enum bel_magic {
	BEL_POWER_ON_STATUS     = 0x53696C12,
	BEL_TIMEOF_DAY_STATUS   = 0x53696CF0,
	BEL_MAX10_SEU_STATUS    = 0x53696CBC,
	BEL_FPGA_SEU_STATUS     = 0x53696CDE,
	BEL_POWER_OFF_STATUS    = 0x53696C34,
	BEL_SENSORS_STATE       = 0x53696C56,
	BEL_SENSORS_STATUS      = 0x53696C78,
	BEL_PCI_ERROR_STATUS    = 0x53696C9A,
	BEL_PCI_V1_ERROR_STATUS = 0x53696D12
};

enum bel_power_regulator {
	BEL_PWR_REG_IR38063_VOUT = 0,
	BEL_PWR_REG_IR38063_IOUT,
	BEL_PWR_REG_IR38063_VIN,
	BEL_PWR_REG_IR38063_TEMP,
	BEL_PWR_REG_ISL68220_VOUT,
	BEL_PWR_REG_ISL68220_IOUT,
	BEL_PWR_REG_ISL68220_VIN,
	BEL_PWR_REG_ISL68220_TEMP
};

struct bel_sensor_info {
	uint32_t id;
	const char *label;
	const char *unit;
	uint32_t resolution;
};

static struct bel_sensor_info bel_sensor_info[] = {
	{ .id =  1, .label = "FPGA E-TILE Max Temperature", .unit = "°C", .resolution = 2 },
	{ .id =  2, .label = "FPGA E-TILE Temperature#1", .unit = "°C", .resolution = 2 },
	{ .id =  3, .label = "FPGA E-TILE Temperature#2", .unit = "°C", .resolution = 2 },
	{ .id =  4, .label = "FPGA E-TILE Temperature#3", .unit = "°C", .resolution = 2 },
	{ .id =  5, .label = "FPGA E-TILE Temperature#4", .unit = "°C", .resolution = 2 },
	{ .id =  6, .label = "FPGA P-TILE Temperature", .unit = "°C", .resolution = 2 },
	{ .id =  7, .label = "FPGA P-TILE DTS Temperature", .unit = "°C", .resolution = 2 },
	{ .id =  8, .label = "FPGA P-TILE Max Temperature", .unit = "°C", .resolution = 2 },
	{ .id =  9, .label = "FPGA FABRIC DTS#1", .unit = "°C", .resolution = 2 },
	{ .id = 10, .label = "FPGA FABRIC DTS#2", .unit = "°C", .resolution = 2 },
	{ .id = 11, .label = "FPGA FABRIC DTS#3", .unit = "°C", .resolution = 2 },
	{ .id = 12, .label = "FPGA FABRIC DTS#4", .unit = "°C", .resolution = 2 },
	{ .id = 13, .label = "FPGA FABRIC DTS#5", .unit = "°C", .resolution = 2 },
	{ .id = 14, .label = "FPGA FABRIC RDTS#1", .unit = "°C", .resolution = 2 },
	{ .id = 15, .label = "FPGA FABRIC RDTS#2", .unit = "°C", .resolution = 2 },
	{ .id = 16, .label = "FPGA FABRIC RDTS#3", .unit = "°C", .resolution = 2 },
	{ .id = 17, .label = "FPGA FABRIC RDTS#4", .unit = "°C", .resolution = 2 },
	{ .id = 18, .label = "Board Bottom", .unit = "°C", .resolution = 2 },
	{ .id = 19, .label = "FPGA Corner (SDM)Temperature", .unit = "°C", .resolution = 2 },
	{ .id = 20, .label = "FPGA Core Fabric Temperature", .unit = "°C", .resolution = 2 },
	{ .id = 21, .label = "FPGA P-Tile Temperature", .unit = "°C", .resolution = 2 },
	{ .id = 22, .label = "FPGA E-Tile Temperature", .unit = "°C", .resolution = 2 },
	{ .id = 23, .label = "Board Top Temperature", .unit = "°C", .resolution = 2 },
	{ .id = 24, .label = "Board Rear Side Temperature", .unit = "°C", .resolution = 2 },
	{ .id = 25, .label = "Board Front Side Temperature", .unit = "°C", .resolution = 2 },
	{ .id = 26, .label = "FPGA Ambient Temperature", .unit = "°C", .resolution = 2 },
	{ .id = 27, .label = "FPGA PTILE2 External Temperature", .unit = "°C", .resolution = 2 },
	{ .id = 28, .label = "QSFP1(Primary) Case Temperature", .unit = "°C", .resolution = 2 },
	{ .id = 29, .label = "QSFP2(Secondary) Case Temperature", .unit = "°C", .resolution = 2 },
	{ .id = 30, .label = "Inlet 12V PCIe Rail [Voltage]", .unit = "mV", .resolution = 1 },
	{ .id = 31, .label = "Inlet 12V PCIe Rail [Current]", .unit = "mA", .resolution = 1 },
	{ .id = 32, .label = "Inlet 12V Aux Rail [Voltage]", .unit = "mV", .resolution = 1 },
	{ .id = 33, .label = "Inlet 12V Aux Rail [Current]", .unit = "mA", .resolution = 1 },
	{ .id = 34, .label = "Inlet 3V3 PCIe Rail [Voltage]", .unit = "mV", .resolution = 1 },
	{ .id = 35, .label = "Inlet 3V3 PCIe Rail [Current]", .unit = "mA", .resolution = 1 },
	{ .id = 36, .label = "Board Power", .unit = "mW", .resolution = 1 },
	{ .id = 37, .label = "QSFP 3V3 Rail [Voltage]", .unit = "mV", .resolution = 1 },
	{ .id = 38, .label = "QSFP 3V3 Rail [Current]", .unit = "mA", .resolution = 1 },
	{ .id = 39, .label = "QSFP(Primary) Supply Voltage Rail[Voltage]", .unit = "mV", .resolution = 1 },
	{ .id = 40, .label = "QSFP(Secondary) Supply Voltage Rail[Voltage]", .unit = "mV", .resolution = 1 },
	{ .id = 41, .label = "Virt FPGA Temperature", .unit = "°C", .resolution = 2 },
	{ .id = 42, .label = "SOC Package Power", .unit = "mW", .resolution = 2 },
	{ .id = 43, .label = "SOC Package Temperature", .unit = "°C", .resolution = 2 },
	{ .id = 44, .label = "FPGA Package Power", .unit = "mW", .resolution = 2 }
};

static struct bel_sensor_info bel_power_regulator_info[] = {
	[BEL_PWR_REG_IR38063_VOUT]  = { .label = "IR38063 Voltage",      .unit = "mV", .resolution = 1 },
	[BEL_PWR_REG_IR38063_IOUT]  = { .label = "IR38063 Current",      .unit = "mA", .resolution = 1 },
	[BEL_PWR_REG_IR38063_VIN]   = { .label = "IR38063 Input",        .unit = "mV", .resolution = 1 },
	[BEL_PWR_REG_IR38063_TEMP]  = { .label = "IR38063 Temperature",  .unit = "°C", .resolution = 1 },
	[BEL_PWR_REG_ISL68220_VOUT] = { .label = "ISL68220 Voltage",     .unit = "mV", .resolution = 1 },
	[BEL_PWR_REG_ISL68220_IOUT] = { .label = "ISL68220 Current",     .unit = "mA", .resolution = 1 },
	[BEL_PWR_REG_ISL68220_VIN]  = { .label = "ISL68220 Input",       .unit = "mV", .resolution = 1 },
	[BEL_PWR_REG_ISL68220_TEMP] = { .label = "ISL68220 Temperature", .unit = "°C", .resolution = 1 }
};

struct pwron_status {
	uint64_t value;
	char str[256];
};

static struct pwron_status pwron_status_info[] = {
	{.value = 0, .str = "Default(not attempted)" },
	{.value = 1, .str = "Under progress" },
	{.value = 2, .str = "Success"},
	{.value = 3, .str = "Failed" },
	{.value = 9, .str = "Repower cycle under progress" },
	{.value = 0xa, .str = "Repower cycle success" },
	{.value = 0xb, .str = "Repower cycle failed" }
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

static void bel_print_pwr_on_sts(const char *label, uint32_t value, size_t first, size_t last)
{
	uint32_t mask = UINT_MAX >> (32 - (last - first));
	uint32_t field = (value >> first) & mask;
	size_t i = 0;
	for (i = 0; i < ARRAY_SIZE(pwron_status_info); i++) {
		if (pwron_status_info[i].value == field) {
			printf("      " BEL_LABEL_FMT "%s(0x%x)\n", 46, label, pwron_status_info[i].str, field);
			return;
		}
	}
	printf("      " BEL_LABEL_FMT "(%s)0x%x\n", 46, label, "reserved", field);
}

static void bel_print_value(const char *label, uint32_t value)
{
	printf("    " BEL_LABEL_FMT "0x%08x\n", 48, label, value);
}

static void bel_print_timeofday(const char *label, struct bel_timeof_day *time_of_day)
{
	char time_str[26] = { 0 };
	time_t time_sec = 0;

	// Timestamps are 64-bit milliseconds:
	uint64_t correct_time = ((uint64_t)time_of_day->header.timespamp_high << 32) +
		time_of_day->header.timestamp_low;

	if (time_of_day->header.timespamp_high == 0) {
		uint64_t offset = ((uint64_t)time_of_day->timeofday_offset_high << 32) +
			time_of_day->timeofday_offset_low;
		correct_time += offset;
	}

	// Convert milliseconds to seconds; no rounding up from 500 milliseconds!
	time_sec = correct_time / 1000UL;

	if (ctime_r(&time_sec, time_str) == NULL) {
		OPAE_ERR("Failed to format time: %s", strerror(errno));
		return;
	}
	printf("  " BEL_LABEL_FMT "%s", 50, label, time_str);
}

static void bel_print_header(const char *label, struct bel_header *header)
{
	// Convert milliseconds to seconds;
	time_t time_sec = (((uint64_t)header->timespamp_high << 32) | header->timestamp_low) / 1000UL;
	char time_str[26] = { 0 };

	if (ctime_r(&time_sec, time_str) == NULL) {
		OPAE_ERR("Failed to format time: %s", strerror(errno));
		return;
	}

	printf("  " BEL_LABEL_FMT "%s", 50, label, time_str);
}

static void reserved_field(const char *label, uint32_t value, size_t first, size_t last)
{
	uint32_t mask = UINT_MAX >> (32 - (last - first));
	uint32_t field = (value >> first) & mask;

	if (label == NULL)
		label = "Reserved";

	if (field != 0)
		printf("      " BEL_LABEL_FMT "*** RESERVED FIELD [%lu:%lu] IS NOT ZERO: 0x%X\n", 46, label, last - 1, first, field);
}

static void reserved_bit(const char *label, uint32_t value, size_t offset)
{
	bool bit = (value >> offset) & 0x1;

	if (label == NULL)
		label = "Reserved";

	if (bit)
		printf("      " BEL_LABEL_FMT "*** RESERVED BIT [%lu] IS NOT ZERO: %d\n", 46, label, offset, bit);
}

void bel_print_power_on_status(struct bel_power_on_status *status, struct bel_timeof_day *timeof_day, bool print_bits)
{
	if (status->header.magic != BEL_POWER_ON_STATUS)
		return;

	if (timeof_day->header.magic != BEL_TIMEOF_DAY_STATUS)
		return;

	/* Power on status is logged immediately after power on.
	Time of the day information is written by SW into a BMC register.
	This write will take some time after power on and hence with Power
	on log there is no timestamp value available.*/
	bel_print_timeofday("Power On Status Time", timeof_day);

	/* Register 0x80 */
	bel_print_value("Status (0x80)",        status->status);
	reserved_field(NULL,                    status->status,  0, 24);
	reserved_field(NULL,                    0xFF,  0, 24);
	reserved_bit(NULL,                      ~0,  22);
	bel_print_pwr_on_sts("Power On Code FPGA",   status->status, 24, 28);
	bel_print_pwr_on_sts("Power On Code ICXD",    status->status, 28, 32);

	/* Register 0xa0 */
	bel_print_value("FPGA Config Status (0xA0)",   status->fpga_status);
	bel_print_field("FPGA Page",            status->fpga_status,  0, 3);
	bel_print_bit("FPGA Configured Page",   status->fpga_status,  3);
	bel_print_bit("FPGA Config Timeline",   status->fpga_status,  4);
	bel_print_bit("Flow Done",              status->fpga_status,  5);
	reserved_field(NULL,                    status->fpga_status,  6, 12);
	bel_print_field("FSM State",            status->fpga_status, 12, 16);
	bel_print_field("Duration",             status->fpga_status, 16, 30);
	reserved_field(NULL, status->fpga_status, 30, 32);

	/* Register 0xa4 */
	bel_print_value("FPGA_Config Sts (0xA4)",  status->fpga_config_status);
	bel_print_field("Config Status",              status->fpga_config_status,  0, 4);
	bel_print_bit("Config FW Seq Fail",           status->fpga_config_status,  4);
	bel_print_field("nStatus Stuck Low Restarts", status->fpga_config_status,  5, 8);
	bel_print_field("Image Fail Recycle Count",   status->fpga_config_status,  8, 10);
	bel_print_field("1st Power On Page number",   status->fpga_config_status, 10, 13);
	bel_print_field("2nd Power On Page number",   status->fpga_config_status, 13, 16);
	bel_print_field("3nd Power On Page number",   status->fpga_config_status, 16, 19);
	reserved_field(NULL,                          status->fpga_config_status, 19, 32);

	/* Register 0x90 */
	bel_print_value("Sequencer Status 1 (0x90)", status->sequencer_status_1);
	if (print_bits) {
		bel_print_field("FSM state of FPGA power sequencer state", status->fpga_config_status, 0, 8);
		bel_print_field("FSM state of FPGA power sequencer (n-1) state", status->fpga_config_status, 8, 16);
		bel_print_field("FSM state of ICXD LCC SOC power sequencer state", status->fpga_config_status, 16, 24);
		bel_print_field("FSM state of FPGA power sequencer (n-1) state", status->fpga_config_status, 24, 32);
	}

	/* Register 0x94 */
	bel_print_value("Sequencer Status 2 (0x94)", status->sequencer_status_2);
	if (print_bits) {
		reserved_field(NULL, status->sequencer_status_2, 0, 2);
		bel_print_bit("fm71_fatal_therm", status->sequencer_status_2, 3);
		bel_print_bit("fpga_therm_shdn_n", status->sequencer_status_2, 4);
		bel_print_bit("fpga_therm_alert_n", status->sequencer_status_2, 5);
		bel_print_bit("board_therm_shdn_n", status->sequencer_status_2, 6);
		bel_print_bit("board_therm_alert_n", status->sequencer_status_2, 7);
		bel_print_bit("fpga_cattrip_n", status->sequencer_status_2, 8);
		bel_print_bit("fpga_vid_alert_n", status->sequencer_status_2, 9);
		bel_print_bit("edge_ina_pwr_shdn_n", status->sequencer_status_2, 10);
		bel_print_bit("qsfpb_pg", status->sequencer_status_2, 11);
		bel_print_bit("qsfpa_pg", status->sequencer_status_2, 12);
		bel_print_bit("fpga_grp3_vccio_1v8_sdm_1v8_pg", status->sequencer_status_2, 13);
		bel_print_bit("fpga_grp3_vcc_1v2_pg", status->sequencer_status_2, 14);
		bel_print_bit("fpga_grp2_ddr4_vpp_2v5_pg", status->sequencer_status_2, 15);
		bel_print_bit("fpga_grp2_vcca_1v8_pg", status->sequencer_status_2, 16);
		bel_print_bit("fpga_grp2_vcch_gxer_1v1_pg", status->sequencer_status_2, 17);
		bel_print_bit("fpga_grp2_vccclk_gxer_2v5_pg", status->sequencer_status_2, 18);
		bel_print_bit("fpga_grp1_vcch_0v9_pg", status->sequencer_status_2, 19);
		bel_print_bit("fpga_grp1_vccl_vid_pg", status->sequencer_status_2, 20);
		bel_print_bit("vcc_5v_pg", status->sequencer_status_2, 21);
		bel_print_bit("vcc_3v3_pg", status->sequencer_status_2, 22);
		bel_print_bit("pcie_3v3_uv_n", status->sequencer_status_2, 23);
		bel_print_bit("pcie_3v3_ov_n", status->sequencer_status_2, 24);
		bel_print_bit("power_class", status->sequencer_status_2, 25);
		bel_print_bit("aux_12v_uv_n", status->sequencer_status_2, 26);
		bel_print_bit("aux_12v_ov_n", status->sequencer_status_2, 27);
		bel_print_bit("aux_12v_efuse_pg", status->sequencer_status_2, 28);
		bel_print_bit("pcie_12v_uv_n", status->sequencer_status_2, 29);
		bel_print_bit("pcie_12v_ov_n", status->sequencer_status_2, 30);
		bel_print_bit("pcie_12v_efuse_pg", status->sequencer_status_2, 31);
	}

	/* Register 0x98 */
	bel_print_value("Power Good Status (0x98)", status->power_good_status);
	if (print_bits) {
		bel_print_bit("soc_icx_p1v8_cpu_pg", status->power_good_status, 0);
		bel_print_bit("soc_icx_pvnn_pch_pg", status->power_good_status, 1);
		bel_print_bit("soc_icx_pvccio_p1v05_pch_pg", status->power_good_status, 2);
		bel_print_bit("soc_icx_pvddq_abc_pg", status->power_good_status, 3);
		bel_print_bit("soc_ddr4_vtt_pg", status->power_good_status, 4);
		bel_print_bit("soc_icx_pvccana_cpu_pg", status->power_good_status, 5);
		bel_print_bit("soc_icx_pvccin_cpu_pg", status->power_good_status, 6);
		bel_print_bit("soc_ddr_dram_pwr_ok", status->power_good_status, 7);
		bel_print_bit("soc_pch_pwr_ok", status->power_good_status, 8);
		bel_print_bit("soc_pwrgood_pch_out", status->power_good_status, 9);
		bel_print_bit("soc_plt_rst_n", status->power_good_status, 10);
		bel_print_bit("soc_thermtrip_n", status->power_good_status, 11);
		bel_print_bit("soc_memtrip_n", status->power_good_status, 12);
		bel_print_bit("soc_caterr_n", status->power_good_status, 13);
		bel_print_bit("soc_sleep_s45_n", status->power_good_status, 14);
	}
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
void bel_print_power_off_status(struct bel_power_off_status *status, bool print_bits)
{
	if (status->header.magic != BEL_POWER_OFF_STATUS)
		return;

	bel_print_header("Power Off Status Time", &status->header);

	/* Register 0xa0 */
	bel_print_value("FPGA_Status (0xA0)",           status->fpga_status);
	bel_print_field("FPGA Page",                    status->fpga_status, 0, 3);
	bel_print_bit("FPGA Configured Page",           status->fpga_status, 3);
	bel_print_bit("FPGA Config Timeline",           status->fpga_status, 4);
	bel_print_bit("Flow Done",                      status->fpga_status, 5);
	reserved_field(NULL, status->fpga_status, 6, 12);
	bel_print_field("FSM State",                    status->fpga_status, 12, 16);
	bel_print_field("Duration",                     status->fpga_status, 16, 30);
	reserved_field(NULL, status->fpga_status, 30, 32);

	/* Register 0xa4 */
	bel_print_value("FPGA_Config Status (0xA4)",      status->fpga_config_status);
	bel_print_field("Config Status",                  status->fpga_config_status, 0, 4);
	bel_print_bit("Config FW Seq Fail",               status->fpga_config_status, 4);
	bel_print_field("nStatus Stuck Low Restarts",     status->fpga_config_status, 5, 8);
	bel_print_field("Image Fail Recycle Count",       status->fpga_config_status, 8, 10);
	bel_print_field("1st Power On Page number",       status->fpga_config_status, 10, 13);
	bel_print_field("2nd Power On Page number",       status->fpga_config_status, 13, 16);
	bel_print_field("3nd Power On Page number",       status->fpga_config_status, 16, 19);
	reserved_field(NULL, status->fpga_config_status, 19, 32);

	/* Register 0x84 */
	bel_print_value("Power Good Record 1 (0x84)", status->record_1);
	if (print_bits) {
		reserved_field(NULL, status->fpga_status, 0, 2);
		bel_print_bit("qsfpb_pg",                            status->record_1, 3);
		bel_print_bit("qsfpa_pg",                            status->record_1, 4);
		bel_print_bit("fpga_grp3_vccio_1v8_sdm_1v8_pg",      status->record_1, 5);
		bel_print_bit("fpga_grp3_vcc_1v2_pg",                status->record_1, 6);
		bel_print_bit("fpga_grp2_ddr4_vpp_2v5_pg",           status->record_1, 7);
		bel_print_bit("fpga_grp2_vcca_1v8_pg",               status->record_1,  8);
		bel_print_bit("fpga_grp2_vcch_gxer_1v1_pg",          status->record_1,  9);
		bel_print_bit("fpga_grp2_vccclk_gxer_2v5_pg",        status->record_1, 10);
		bel_print_bit("fpga_grp1_vcch_0v9_pg",               status->record_1, 11);
		bel_print_bit("fpga_grp1_vccl_vid_pg",               status->record_1, 12);
		bel_print_bit("vcc_5v_pg",                           status->record_1, 13);
		bel_print_bit("pcie_3v3_uv_n",                       status->record_1, 14);
		bel_print_bit("pcie_3v3_ov_n",                       status->record_1, 15);
		bel_print_bit("vcc_3v3_pg",                          status->record_1, 16);
		bel_print_bit("aux_12v_uv_n",                        status->record_1, 17);
		bel_print_bit("aux_12v_ov_n",                        status->record_1, 18);
		bel_print_bit("pcie_12v_uv_n",                       status->record_1, 19);
		bel_print_bit("pcie_12v_ov_n",                       status->record_1, 20);
		bel_print_bit("aux_12v_efuse_pg",                    status->record_1, 21);
		bel_print_bit("pcie_12v_efuse_pg",                   status->record_1, 22);
		bel_print_bit("aux_12v_efuse_pg",                    status->record_1, 23);
		bel_print_bit("pcie_12v_efuse_pg",                   status->record_1, 24);
		reserved_field(NULL, status->fpga_status, 24, 25);
		bel_print_bit("fm71_fatal_therm",                    status->record_1, 27);
		bel_print_bit("fpga_cattrip_n",                      status->record_1, 28);
		bel_print_bit("edge_ina_pwr_shdn_n",                 status->record_1, 29);
		bel_print_bit("fpga_therm_shdn_n",                   status->record_1, 30);
		bel_print_bit("board_therm_shdn_n",                  status->record_1, 31);
	}

	/* Register 0x88 */
	bel_print_value("Power Good Record 2 (0x88)", status->record_2);
	if (print_bits) {
		bel_print_bit("soc_pchhot_n",                        status->record_2,  0);
		bel_print_bit("soc_memhot_out_n",                    status->record_2,  1);
		bel_print_bit("soc_memtrip_n",                       status->record_2,  2);
		bel_print_bit("soc_thermtrip_n",                     status->record_2,  3);
		bel_print_bit("soc_icx_pvccin_cpu_pg",               status->record_2,  4);
		bel_print_bit("soc_icx_pvccana_cpu_pg",              status->record_2, 5);
		bel_print_bit("soc_ddr4_vtt_pg",                     status->record_2, 6);
		bel_print_bit("soc_icx_pvddq_abc_pg",                status->record_2, 7);
		bel_print_bit("soc_icx_pvnn_pch_pg",                 status->record_2, 9);
		bel_print_bit("soc_icx_p1v8_cpu_pg",                 status->record_2, 10);
		reserved_field(NULL,                status->record_2,  11, 32);
	}

	/* Register 0x50 */
	bel_print_value("GPI Status (0x50)", status->general_purpose_input_status);
	if (print_bits) {
		bel_print_bit("Board revision strap LSB, hard strap",       status->general_purpose_input_status,  0);
		bel_print_bit("Board revision strap",                       status->general_purpose_input_status,  1);
		bel_print_bit("Power class 0 strap",                        status->general_purpose_input_status,  2);
		bel_print_bit("Power class 1 strap",                        status->general_purpose_input_status,  3);
		bel_print_bit("SM bus I2C address bit setting, hard strap", status->general_purpose_input_status,  4);
		bel_print_bit("CVL present indication, hard strap",         status->general_purpose_input_status,  5);
		bel_print_bit("Alert indication from PM bus VRs",           status->general_purpose_input_status,  6);
		bel_print_bit("FM61 SDM VID alertn output",                 status->general_purpose_input_status,  7);
		bel_print_bit("Output from ED8401",                         status->general_purpose_input_status,  8);
		bel_print_bit("Warning alert output from TMP464",           status->general_purpose_input_status,  9);
		bel_print_bit("Output from INA3221",                        status->general_purpose_input_status, 10);
		bel_print_bit("Loss of input reference to SI53254",         status->general_purpose_input_status, 11);
		bel_print_bit("Loss of lLock indication from Zarlink",      status->general_purpose_input_status, 12);
		bel_print_bit("Loss of Lock Indication from Si5392",        status->general_purpose_input_status, 13);
		reserved_bit(NULL,                                          status->general_purpose_input_status, 14);
		bel_print_bit("Interrupt from IO Expander",                 status->general_purpose_input_status, 15);
		bel_print_bit("Output from INA3221",                        status->general_purpose_input_status, 16);
		bel_print_bit("QSFP-A module presence",                     status->general_purpose_input_status, 17);
		bel_print_bit("QSFP-B module presence",                     status->general_purpose_input_status, 18);
		reserved_field(NULL,                                        status->general_purpose_input_status, 19, 32);
	}

	/* Register 0x410 */
	bel_print_value("Sensor Failed (0x410)", status->sensor_failed);
	if (print_bits) {
		bel_print_pass("fpga_remote_temp",        status->sensor_failed, 2);
		bel_print_pass("board_temp",              status->sensor_failed, 2);
		bel_print_pass("inlet_12v_pcie",          status->sensor_failed, 2);
		bel_print_pass("fpga_vcch_gxer",          status->sensor_failed, 3);
		bel_print_pass("max10_board_clk_pwr",     status->sensor_failed, 4);
		bel_print_pass("icxd_voltage",            status->sensor_failed, 5);
		bel_print_pass("icxd_voltage",            status->sensor_failed, 6);
		bel_print_pass("fpga_vcch",               status->sensor_failed, 7);
		bel_print_pass("fpga_core_vol_pwr_temp",  status->sensor_failed, 8);
		bel_print_pass("fpga_fab_tile_temp",      status->sensor_failed, 9);
		bel_print_pass("qsfp1_sts",               status->sensor_failed, 10);
		bel_print_pass("qsfp2_sts",               status->sensor_failed, 11);
		bel_print_pass("io_expander_sts",         status->sensor_failed, 12);
		bel_print_pass("qsfp1_controller_access", status->sensor_failed, 13);
		bel_print_pass("qsfp1_module_plugged",    status->sensor_failed, 14);
		bel_print_pass("qsfp1_module_supported",  status->sensor_failed, 15);
		bel_print_pass("qsfp1_diag_data_avl",     status->sensor_failed, 16);
		bel_print_pass("qsfp2_controller_access", status->sensor_failed, 17);
		bel_print_pass("qsfp2_module_plugged",    status->sensor_failed, 18);
		bel_print_pass("qsfp2_module_supported",  status->sensor_failed, 19);
		bel_print_pass("qsfp2_diag_data_avl",     status->sensor_failed, 20);
		bel_print_pass("ADS7128_I2C_SA15H",       status->sensor_failed, 21);
		bel_print_pass("ADS7128_I2C_SA16H",       status->sensor_failed, 22);
		bel_print_pass("ADS7128_I2C_SA17H",       status->sensor_failed, 23);
		bel_print_pass("ICXD_LCC_SMBUS_PECI",     status->sensor_failed, 24);
		bel_print_pass("ICXD_LCC_IR38163",        status->sensor_failed, 25);
		bel_print_pass("ICXD_LCC_IR38363",        status->sensor_failed, 26);
		bel_print_pass("PXE1410_ICXD_LCC",        status->sensor_failed, 27);
		bel_print_pass("PXM1310_ICXD_LCC",        status->sensor_failed, 28);
		reserved_field(NULL,                      status->sensor_failed, 29, 30);
		bel_print_pass("overall_devices",         status->sensor_failed, 31);
	}

	/* Register 0x414 */
	bel_print_value("Sensor Alert 1 (0x414)", status->sensor_alert_1);

	if (print_bits)
		bel_print_sensor_alert(status->sensor_alert_1, 0);

	/* Register 0x418 */
	bel_print_value("Sensor Alert 2 (0x418)", status->sensor_alert_2);

	if (print_bits)
		bel_print_sensor_alert(status->sensor_alert_1, 32);

	/* Register 0x41c */
	bel_print_value("Sensor Alert 3 (0x41C)", status->sensor_alert_3);

	if (print_bits)
		bel_print_sensor_alert(status->sensor_alert_1, 40);
}

size_t bel_print_sensor(struct bel_sensor_state *state, size_t last)
{
	struct bel_sensor_info *info = NULL;
	size_t next = last + 1;

	/* Search the info array starting from one past the previous printed sensor */
	while (next != last) {
		info = &bel_sensor_info[next];

		if (info->id == state->id)
			break;

		if (state->id == 0)
			break;

		next = (next + 1) % ARRAY_SIZE(bel_sensor_info);
	}

	/* Print nothing if sensor wasn't found */
	if (next == last)
		return last;

	if (info->id == 0)
		return last;
	printf("    " BEL_LABEL_FMT, 48, info->label);
	if (state->reading != INT_MAX)
		printf("%6u %s\n", state->reading / info->resolution, info->unit);
	else
		printf("%9s\n", "N/A");

	return next;
}

void bel_print_sensors_state(struct bel_sensors_state *state)
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

void bel_print_sensors_status(struct bel_sensors_status *status)
{
	if (status->header.magic != BEL_SENSORS_STATUS)
		return;

	bel_print_header("Sensor Status Time", &status->header);

	bel_print_value("INA3221 Reg 0xF @i2c Addr 0x40 ", status->ina3221_1_mask_enable);
	bel_print_value("INA3221 Reg 0xF @i2c Addr 0x41", status->ina3221_2_mask_enable);
	bel_print_value("INA3221 Reg 0xF @i2c Addr 0x42", status->ina3221_3_mask_enable);
	bel_print_sensors_status_ext("PMBUS IR38063", &status->ir38063, BEL_PWR_REG_IR38063_VOUT);
	bel_print_sensors_status_ext("PMBUS ISL68220", &status->isl68220, BEL_PWR_REG_ISL68220_VOUT);
	bel_print_value("ED8401 Status", status->ed8401_status);
}

void bel_print_max10_seu(struct bel_max10_seu *status)
{
	if (status->header.magic != BEL_MAX10_SEU_STATUS)
		return;
	bel_print_header("Max10 SEU Time", &status->header);
	bel_print_bit("MAX10 SEU error status", status->max10_seu, 0);

}

void bel_print_timeof_day(struct bel_timeof_day *timeof_day)
{
	if (timeof_day->header.magic != BEL_TIMEOF_DAY_STATUS)
		return;

	bel_print_header("Time of day", &timeof_day->header);
	bel_print_timeofday("Time of day offset", timeof_day);

	bel_print_value("TimeOfDay offset low", timeof_day->timeofday_offset_low);
	bel_print_value("TimeOfDay offset high", timeof_day->timeofday_offset_high);
}

void bel_print_fpga_seu(struct bel_fpga_seu *status)
{
	if (status->header.magic != BEL_FPGA_SEU_STATUS)
		return;

	bel_print_header("FPGA SEU Time", &status->header);
	bel_print_bit("FPGA SEU error status", status->fpga_seu, 1);
}

void bel_print_pci_error_status(struct bel_pci_error_status *status, bool print_bits)
{
	if (status->header.magic != BEL_PCI_ERROR_STATUS)
		return;

	bel_print_header("PCI Error Status Time", &status->header);

	// PCIe Link Status
	bel_print_value("PCIe Link Status", status->pcie_link_status);
	if (print_bits) {
		bel_print_field("Current Link Speed", status->pcie_link_status, 0, 3);
		bel_print_field("Negotiated Link Speed", status->pcie_link_status, 4, 9);
		bel_print_bit("Link Training ", status->pcie_link_status, 11);
		bel_print_bit("Slot Clock Configuration", status->pcie_link_status, 12);
		bel_print_bit("Data link layer link active", status->pcie_link_status, 13);
		bel_print_bit("Link Bandwidth Management Status", status->pcie_link_status, 14);
		bel_print_bit("Link Autonomous Management Status", status->pcie_link_status, 15);
	}

	// PCIe Uncorrectable Error
	bel_print_value("PCIe Uncorrectable Error", status->pcie_uncorr_err);
	if (print_bits) {
		bel_print_bit("Data Link Protocol error Status", status->pcie_uncorr_err, 4);
		bel_print_bit("Surprise down error Status", status->pcie_uncorr_err, 5);
		bel_print_bit("Poisoned TLP received", status->pcie_uncorr_err, 12);
		bel_print_bit("Flow Control Protocol Errors Status", status->pcie_uncorr_err, 13);
		bel_print_bit("Completion Timeout Status", status->pcie_uncorr_err, 14);
		bel_print_bit("Completer Abort error Status", status->pcie_uncorr_err, 15);
		bel_print_bit("Unexpected Completion Status", status->pcie_uncorr_err, 16);
		bel_print_bit("Receiver Overflow Status", status->pcie_uncorr_err, 17);
		bel_print_bit("Malformed TLP Status", status->pcie_uncorr_err, 18);
		bel_print_bit("ECRC Error Status", status->pcie_uncorr_err, 19);
		bel_print_bit("Unsupported Request Error Status", status->pcie_uncorr_err, 20);
		bel_print_bit("ACS Violation Status", status->pcie_uncorr_err, 21);
		bel_print_bit("Uncorrectable Internal Error Status", status->pcie_uncorr_err, 22);
		bel_print_bit("MC Blocked TLP Status", status->pcie_uncorr_err, 23);
		bel_print_bit("AtomicOp Egress Blocked Status", status->pcie_uncorr_err, 24);
		bel_print_bit("TLP Prefix Blocked Status", status->pcie_uncorr_err, 25);
		bel_print_bit("Poisoned TLP Egress Blocked Status", status->pcie_uncorr_err, 26);
	}

}

void bel_print_pci_v1_error_status(struct bel_pcie_v1_error_status *status, bool print_bits)
{

	if (status->header.magic != BEL_PCI_V1_ERROR_STATUS)
		return;

	bel_print_header("PCI Error Status Time", &status->header);

	// PCIe Link Status
	bel_print_value("PCIe Link Status", status->pcie_link_status);
	if (print_bits) {
		bel_print_field("Current Link Speed", status->pcie_link_status, 0, 3);
		bel_print_field("Negotiated Link Speed", status->pcie_link_status, 4, 9);
		bel_print_bit("Link Training ", status->pcie_link_status, 11);
		bel_print_bit("Slot Clock Configuration", status->pcie_link_status, 12);
		bel_print_bit("Data link layer link active", status->pcie_link_status, 13);
		bel_print_bit("Link Bandwidth Management Status", status->pcie_link_status, 14);
		bel_print_bit("Link Autonomous Management Status", status->pcie_link_status, 15);
	}

	// PCIe Uncorrectable Error
	bel_print_value("PCIe Uncorrectable Error", status->pcie_uncorr_err);
	if (print_bits) {
		bel_print_bit("Data Link Protocol error Status", status->pcie_uncorr_err, 4);
		bel_print_bit("Surprise down error Status", status->pcie_uncorr_err, 5);
		bel_print_bit("Poisoned TLP received", status->pcie_uncorr_err, 12);
		bel_print_bit("Flow Control Protocol Errors Status", status->pcie_uncorr_err, 13);
		bel_print_bit("Completion Timeout Status", status->pcie_uncorr_err, 14);
		bel_print_bit("Completer Abort error Status", status->pcie_uncorr_err, 15);
		bel_print_bit("Unexpected Completion Status", status->pcie_uncorr_err, 16);
		bel_print_bit("Receiver Overflow Status", status->pcie_uncorr_err, 17);
		bel_print_bit("Malformed TLP Status", status->pcie_uncorr_err, 18);
		bel_print_bit("ECRC Error Status", status->pcie_uncorr_err, 19);
		bel_print_bit("Unsupported Request Error Status", status->pcie_uncorr_err, 20);
		bel_print_bit("ACS Violation Status", status->pcie_uncorr_err, 21);
		bel_print_bit("Uncorrectable Internal Error Status", status->pcie_uncorr_err, 22);
		bel_print_bit("MC Blocked TLP Status", status->pcie_uncorr_err, 23);
		bel_print_bit("AtomicOp Egress Blocked Status", status->pcie_uncorr_err, 24);
		bel_print_bit("TLP Prefix Blocked Status", status->pcie_uncorr_err, 25);
		bel_print_bit("Poisoned TLP Egress Blocked Status", status->pcie_uncorr_err, 26);
	}

	// PCIe Uncorrectable Err Mask
	bel_print_value("PCIe Uncorrectable Err Mask", status->pcie_uncorr_err_mask);
	if (print_bits) {
		bel_print_bit("Data Link Protocol error", status->pcie_uncorr_err, 4);
		bel_print_bit("Surprise down error", status->pcie_uncorr_err, 5);
		bel_print_bit("Poisoned TLP received", status->pcie_uncorr_err, 12);
		bel_print_bit("Flow Control Protocol Errors", status->pcie_uncorr_err, 13);
		bel_print_bit("Completion Timeout", status->pcie_uncorr_err, 14);
		bel_print_bit("Completer Abort error", status->pcie_uncorr_err, 15);
		bel_print_bit("Unexpected Completion", status->pcie_uncorr_err, 16);
		bel_print_bit("Receiver Overflow", status->pcie_uncorr_err, 17);
		bel_print_bit("Malformed TLP", status->pcie_uncorr_err, 18);
		bel_print_bit("ECRC Error", status->pcie_uncorr_err, 19);
		bel_print_bit("Unsupported Request Error", status->pcie_uncorr_err, 20);
		bel_print_bit("ACS Violation", status->pcie_uncorr_err, 21);
		bel_print_bit("Uncorrectable Internal Error", status->pcie_uncorr_err, 22);
		bel_print_bit("MC Blocked TLP", status->pcie_uncorr_err, 23);
		bel_print_bit("AtomicOp Egress Blocked", status->pcie_uncorr_err, 24);
		bel_print_bit("TLP Prefix Blocked", status->pcie_uncorr_err, 25);
		bel_print_bit("Poisoned TLP Egress Blocked", status->pcie_uncorr_err, 26);
	}

	//PCIE Uncorrectable Err Severity
	bel_print_value("PCIe Uncorrectable Err Severity", status->pcie_uncorr_err_severity);
	if (print_bits) {
		bel_print_bit("Data Link Protocol error", status->pcie_uncorr_err_severity, 4);
		bel_print_bit("Surprise Down Error", status->pcie_uncorr_err_severity, 5);
		bel_print_bit("Poisoned TLP", status->pcie_uncorr_err_severity, 12);
		bel_print_bit("Flow Control protocol error", status->pcie_uncorr_err_severity, 13);
		bel_print_bit("Completion Timeout", status->pcie_uncorr_err_severity, 14);
		bel_print_bit("Completer Abort (CA) was transmitted", status->pcie_uncorr_err_severity, 15);
		bel_print_bit("Unexpected Completion was received", status->pcie_uncorr_err_severity, 16);
		bel_print_bit("Receiver Overflow", status->pcie_uncorr_err_severity, 17);
		bel_print_bit("Malformed TLP Received", status->pcie_uncorr_err_severity, 18);
		bel_print_bit("ECRC Error Detected", status->pcie_uncorr_err_severity, 19);
		bel_print_bit("Unsupported Request Received", status->pcie_uncorr_err_severity, 20);
	}

	//PCIE Correctable Err Status
	bel_print_value("PCIe Correctable Err Status", status->pcie_corr_err_status);
	if (print_bits) {
		bel_print_bit("Receiver Error status", status->pcie_corr_err_status, 0);
		bel_print_bit("Bad TLP status", status->pcie_corr_err_status, 6);
		bel_print_bit("Bad DLLP status", status->pcie_corr_err_status, 7);
		bel_print_bit("Replay Number Rollover status", status->pcie_corr_err_status, 8);
		bel_print_bit("Replay timer Timeout status", status->pcie_corr_err_status, 12);
		bel_print_bit("Advisory Non-Fatal Error status", status->pcie_corr_err_status, 13);
		bel_print_bit("Corrected internal error status", status->pcie_corr_err_status, 14);
	}

	//PCIE Correctable Err Mask
	bel_print_value("PCIe Correctable Err Mask", status->pcie_corr_err_mask);
	if (print_bits) {
		bel_print_bit("Receiver Error", status->pcie_corr_err_status, 0);
		bel_print_bit("Bad TLP", status->pcie_corr_err_status, 6);
		bel_print_bit("Bad DLLP", status->pcie_corr_err_status, 7);
		bel_print_bit("Replay Number Rollover", status->pcie_corr_err_status, 8);
		bel_print_bit("Replay timer Timeout", status->pcie_corr_err_status, 12);
		bel_print_bit("Advisory Non-Fatal Error", status->pcie_corr_err_status, 13);
		bel_print_bit("Corrected internal error", status->pcie_corr_err_status, 14);
	}

	//PCIE Cap And Ctrl
	bel_print_value("PCIe Cap And Ctrl", status->pcie_cap_ctrl);

	//PCIE Header Log DW 1
	bel_print_value("PCIE Header Log DW1", status->pcie_header_log1);
	//PCIE Header Log DW 2
	bel_print_value("PCIE Header Log DW2", status->pcie_header_log1);
	//PCIE Header Log DW 3
	bel_print_value("PCIE Header Log DW3", status->pcie_header_log1);
	//PCIE Header Log DW 4
	bel_print_value("PCIE Header Log DW4", status->pcie_header_log1);

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
	bel_print_power_on_status(&event->power_on_status, &event->timeof_day, print_bits);
	bel_print_timeof_day(&event->timeof_day);
	bel_print_max10_seu(&event->max10_seu);
	bel_print_fpga_seu(&event->fpga_seu);
	bel_print_pci_error_status(&event->pci_error_status, print_bits);

	bel_print_power_off_status(&event->power_off_status, print_bits);

	if (print_sensors) {
		bel_print_sensors_state(&event->sensors_state);
		bel_print_sensors_status(&event->sensors_status);
	}
	bel_print_pci_v1_error_status(&event->pcie_v1_error_status, print_bits);

}

void bel_timespan(struct bel_event *event, uint32_t idx)
{
	struct bel_header *header_off = &event->power_off_status.header;
	time_t off_sec = (((uint64_t)header_off->timespamp_high << 32) |
		header_off->timestamp_low) /1000UL;
	char off_str[26] = { 'N', '/', 'A', '\0' };
	char on_str[26] = { '\0' };
	time_t on_sec = 0;

	/* Power on status is logged immediately after power on.
	Time of the day information is written by SW into a BMC register.
	This write will take some time after power on and hence with Power
	on log there is no timestamp value available.*/
	if (event->timeof_day.header.magic != BEL_TIMEOF_DAY_STATUS)
		return;

	// Timestamps are 64-bit milliseconds:
	uint64_t correct_time = ((uint64_t)event->timeof_day.header.timespamp_high << 32) +
		event->timeof_day.header.timestamp_low;

	if (event->timeof_day.header.timespamp_high == 0) {
		uint64_t offset = ((uint64_t)event->timeof_day.timeofday_offset_high << 32) +
			event->timeof_day.timeofday_offset_low;
		correct_time += offset;
	}

	// Convert milliseconds to seconds; no rounding up from 500 milliseconds!
	on_sec = correct_time / 1000UL;

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

	if (idx == 0) {
		printf("%-15s : %-25s : %-25s\n", "Boot Index", "Power-ON Timestamp", "Power-OFF Timestamp");
		printf("-------------------------------------------------------------------------\n");
		printf("%-15s - %-20s  - %-20s\n", "Current Boot", on_str, off_str);
	} else {
		printf("Boot %-10u - %-20s  - %-20s\n", idx, on_str, off_str);
	}

}

bool bel_empty(struct bel_event *event)
{
	return event->power_on_status.header.magic == UINT_MAX;
}

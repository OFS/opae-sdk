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

/*
 * bmc_thermal.c : handles NULL bitstream programming on BMC_THERMAL
 */

#include <opae/fpga.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <float.h>
#include <math.h>
#include "bmc_thermal.h"
#include "config_int.h"
#include "log.h"
#include "bitstream_int.h"
#include "safe_string/safe_string.h"
#include "bmc/bmc.h"
#include "reset_bmc.h"

/*
 * macro to check FPGA return codes, print error message, and goto cleanup label
 * NOTE: this changes the program flow (uses goto)!
 */
#define ON_GOTO(cond, label, desc, ...)                                        \
	do {                                                                   \
		if (cond) {                                                    \
			dlog("pacd[%d]: " desc "\n", ctx.c->PAC_index,         \
			     ##__VA_ARGS__);                                   \
			goto label;                                            \
		}                                                              \
	} while (0)

static void setBMCUNR(bmc_sdr_handle sdr_h, int sens, double value)
{
	fpga_result res = FPGA_OK;
	threshold_list thresh;

	memset_s(&thresh, sizeof(thresh), 0);
	thresh.upper_nr_thresh.is_valid = 1;
	thresh.upper_nr_thresh.value = value;

	res = bmcSetHWThresholds(sdr_h, sens, &thresh);
	if (FPGA_OK != res) {
		dlog("Error setting BMC UNR HW threshold for sensor %d\n",
		     sens);
	}
}

// TODO:  Implement these - send IPMI to BMC
static void setBMCLNR(bmc_sdr_handle sdr_h, int sens, double value)
{
	fpga_result res = FPGA_OK;
	threshold_list thresh;

	memset_s(&thresh, sizeof(thresh), 0);
	thresh.lower_nr_thresh.is_valid = 1;
	thresh.lower_nr_thresh.value = value;

	res = bmcSetHWThresholds(sdr_h, sens, &thresh);
	if (FPGA_OK != res) {
		dlog("Error setting BMC LNR HW threshold for sensor %d\n",
		     sens);
	}
}

static void setUpperTrigger(int sens_num, pacd_bmc_reset_context *ctx,
			    double UNR, int thresh_ndx, sdr_details *details)
{
	if (!details->thresholds.upper_nr_thresh.is_valid) {
		ctx->c->config->upper_trigger_value[thresh_ndx] = DBL_MAX;
		ctx->c->config->upper_reset_value[thresh_ndx] = -DBL_MAX;
		return;
	}

	ctx->c->config->upper_trigger_value[thresh_ndx] = UNR;
	ctx->c->config->upper_reset_value[thresh_ndx] = UNR;

	if (details->type == BMC_THERMAL) {
		setBMCUNR(ctx->records, sens_num, UNR + PACD_THERMAL_INCREMENT);
	}

	if (details->type == BMC_POWER) {
		setBMCUNR(ctx->records, sens_num, UNR * PACD_POWER_MULTIPLIER);
	}
}

static void setLowerTrigger(int sens_num, pacd_bmc_reset_context *ctx,
			    double LNR, int thresh_ndx, sdr_details *details)
{
	if (!details->thresholds.lower_nr_thresh.is_valid) {
		ctx->c->config->lower_trigger_value[thresh_ndx] = -DBL_MAX;
		ctx->c->config->lower_reset_value[thresh_ndx] = DBL_MAX;
		return;
	}

	ctx->c->config->lower_trigger_value[thresh_ndx] = LNR;
	ctx->c->config->lower_reset_value[thresh_ndx] = LNR;

	if (details->type == BMC_THERMAL) {
		setBMCLNR(ctx->records, sens_num, LNR - PACD_THERMAL_INCREMENT);
	}

	if (details->type == BMC_POWER) {
		setBMCLNR(ctx->records, sens_num, LNR / PACD_POWER_MULTIPLIER);
	}
}

static void setSensorDefaults(pacd_bmc_reset_context *ctx)
{
	int sens_num = 0;
	int thresh_ndx = 0;
	uint32_t num_sensors = ctx->num_sensors;
	uint32_t num_thresh = ctx->c->config->num_thresholds;
	struct config *cfg = ctx->c->config;
	sdr_details details;
	fpga_result res = FPGA_OK;
	uint32_t num_values;
	uint32_t is_valid;
	double tmp;

	if (cfg->no_defaults) {
		return;
	}

	res = bmcReadSensorValues(ctx->records, &ctx->values, &num_values);
	if (res != FPGA_OK) {
		dlog("ERROR: Cannot read sensor values while setting defaults\n");
		return;
	}

	for (sens_num = 0; sens_num < (int)num_sensors; sens_num++) {
		res = bmcGetSensorReading(ctx->values, sens_num, &is_valid,
					  &tmp);
		if (res != FPGA_OK) {
			dlog("ERROR: Cannot get sensor reading while setting defaults\n");
			continue;
		}
		if (!is_valid) {
			continue;
		}

		res = bmcGetSDRDetails(ctx->values, sens_num, &details);
		if (res != FPGA_OK) {
			dlog("ERROR: Cannot read sensor details while setting defaults\n");
			continue;
		}

		double UNR = DBL_MAX;
		double LNR = DBL_MAX;

		// If thresholds unavailable - continue
		if (details.thresholds.upper_nr_thresh.is_valid) {
			UNR = details.thresholds.upper_nr_thresh.value;
		}
		if (details.thresholds.lower_nr_thresh.is_valid) {
			LNR = details.thresholds.lower_nr_thresh.value;
		}
		if ((DBL_MAX == UNR) && (DBL_MAX == LNR)) {
			continue;
		}

		num_thresh = ctx->c->config->num_thresholds;
		for (thresh_ndx = 0; thresh_ndx < (int)num_thresh;
		     thresh_ndx++) {
			if (cfg->sensor_number[thresh_ndx] == sens_num) {
				// Value specified on command-line
				if ((UNR != DBL_MAX)
				    && (cfg->upper_trigger_value[thresh_ndx]
					== DBL_MAX)) {
					setUpperTrigger(sens_num, ctx, UNR,
							thresh_ndx, &details);
				}
				if ((LNR != DBL_MAX)
				    && (cfg->lower_trigger_value[thresh_ndx]
					== -DBL_MAX)) {
					setLowerTrigger(sens_num, ctx, LNR,
							thresh_ndx, &details);
				}
				break;
			}
		}
		if (thresh_ndx == (int)num_thresh) {
			// Not found - add the defaults
			if (UNR != DBL_MAX) {
				setUpperTrigger(sens_num, ctx, UNR, num_thresh,
						&details);
			}
			if (LNR != DBL_MAX) {
				setLowerTrigger(sens_num, ctx, LNR, num_thresh,
						&details);
			}
			ctx->c->config->sensor_number[num_thresh] = sens_num;
			ctx->c->config->num_thresholds++;
		}
	}

	bmcDestroySensorValues(&ctx->values);
}

void *bmc_thermal_thread(void *thread_context)
{
	pacd_bmc_reset_context ctx;
	uint32_t num_values = 0;

	fpga_handle fme_handle;
	fpga_result res;

	memset_s(&ctx, sizeof(ctx), 0);

	ctx.c = (struct bmc_thermal_context *)thread_context;

	ON_GOTO(ctx.c->config->num_null_gbs == 0, out_exit,
		"no default bitstreams registered.");

	res = pacd_bmc_reinit(&ctx);
	ON_GOTO(FPGA_OK != res, out_exit, "Problem initializing.");

	/* if we didn't find a matching FPGA, bail out */
	if (!ctx.gbs_found) {
		dlog("pacd[%d]: no suitable default bitstream for device\n",
		     ctx.c->PAC_index);
		goto out_exit;
	}

	// Set default sensor values to monitor
	setSensorDefaults(&ctx);

	/* fme_token holds the token for an FPGA on our socket matching the
	 * interface ID of the default GBS */
	dlog("pacd[%d]: Sensors monitored:\n", ctx.c->PAC_index);
	unsigned int tnum;
	for (tnum = 0; tnum < ctx.c->config->num_thresholds; tnum++) {
		char ut[512];
		char ur[512];
		char lt[512];
		char lr[512];

		int32_t snum = ctx.c->config->sensor_number[tnum];

		if (ctx.c->config->upper_trigger_value[tnum] != DBL_MAX) {
			snprintf(ut, 512, "%7.2f",
				 ctx.c->config->upper_trigger_value[tnum]);
		} else {
			strcpy_s(ut, 4, "N/A");
		}
		if (ctx.c->config->upper_reset_value[tnum] != -DBL_MAX) {
			snprintf(ur, 512, "%7.2f",
				 ctx.c->config->upper_reset_value[tnum]);
		} else {
			strcpy_s(ur, 4, "N/A");
		}
		if (ctx.c->config->lower_trigger_value[tnum] != -DBL_MAX) {
			snprintf(lt, 512, "%7.2f",
				 ctx.c->config->lower_trigger_value[tnum]);
		} else {
			strcpy_s(lt, 4, "N/A");
		}
		if (ctx.c->config->lower_reset_value[tnum] != DBL_MAX) {
			snprintf(lr, 512, "%7.2f",
				 ctx.c->config->lower_reset_value[tnum]);
		} else {
			strcpy_s(lr, 4, "N/A");
		}

		dlog("pacd[%d]:\tSensor %3d '%-20s': UNR %7s:%7s, LNR %7s:%7s\n",
		     ctx.c->PAC_index, snum, ctx.sensor_names[tnum], ut, ur, lt,
		     lr);
	}

	dlog("pacd[%d]: waiting for tripped threshold(s), "
	     "will write the following bitstream: \"%s\"\n",
	     ctx.c->PAC_index, ctx.c->config->null_gbs[ctx.gbs_index]);

	uint32_t num_sensors = ctx.num_sensors;

	ctx.s_state.last_state = (uint8_t *)calloc((num_sensors + 7) / 8, 1);
	ctx.s_state.tripped = (uint8_t *)calloc((num_sensors + 7) / 8, 1);

	if (!ctx.s_state.last_state || !ctx.s_state.tripped) {
		dlog("pacd[%d]: calloc failed\n", ctx.c->PAC_index);
		goto out_exit;
	}

	while (ctx.c->config->running) {
		uint32_t i;
		double sensor_value;
		uint32_t must_PR = 0;
		uint32_t positive_transition = 0;
		uint32_t negative_transition = 0;
		int retries = 0;

		/* wait for event */
		while ((res = bmcReadSensorValues(ctx.records, &ctx.values,
						  &num_values))
		       != FPGA_OK) {
			retries++;
			if ((retries % 20) == 0) {
				dlog("pacd[%d]: Sensor reading failed.  Retries: %d.\n",
				     ctx.c->PAC_index, retries);
				usleep(900 * 1000);
			}
		}

		for (i = 0; i < ctx.c->config->num_thresholds; i++) {
			int32_t sens_num = ctx.c->config->sensor_number[i];
			double u_trig_val =
				ctx.c->config->upper_trigger_value[i];
			double u_reset_val =
				ctx.c->config->upper_reset_value[i];
			double l_trig_val =
				ctx.c->config->lower_trigger_value[i];
			double l_reset_val =
				ctx.c->config->lower_reset_value[i];
			uint32_t is_valid = 0;

			// Check if sensor disabled due to too many invalid
			// reads
			if (sens_num < 0) {
				continue;
			}

			res = bmcGetSensorReading(ctx.values, sens_num,
						  &is_valid, &sensor_value);
			ON_GOTO(res != FPGA_OK, out_destroy_values,
				"BMC Sensor reading could not be obtained "
				"for sensor %d (%s)",
				sens_num, ctx.sensor_names[sens_num]);

			if (!is_valid) {
				dlog("pacd[%d]: WARNING: Sensor reading for "
				     "sensor %d invalid\n",
				     ctx.c->PAC_index, sens_num);
				ctx.c->config->invalid_count[i]++;
				if (ctx.c->config->invalid_count[i]
				    > DISABLE_THRESHOLD) {
					dlog("pacd[%d]: ERROR: Invalid sensor reading "
					     "threshold for sensor %d exceeded\n",
					     ctx.c->PAC_index, sens_num);
					dlog("pacd[%d]: ERROR: Sensor %d **DISABLED**\n",
					     ctx.c->PAC_index, sens_num);
					ctx.c->config->sensor_number[i] = -1;
				}
				continue;
			}

			if ((sensor_value > u_trig_val)
			    || (sensor_value < l_trig_val)) {
				SET_BIT(ctx.s_state.tripped, sens_num);
				if (!BIT_SET(ctx.s_state.last_state,
					     sens_num)) {
					dlog("pacd[%d]: sensor %d (%s) tripped %s threshold. "
					     "sensor: %f, threshold: %f\n",
					     ctx.c->PAC_index, sens_num,
					     ctx.sensor_names[sens_num],
					     sensor_value > u_trig_val
						     ? "upper"
						     : "lower",
					     sensor_value,
					     sensor_value > u_trig_val
						     ? u_trig_val
						     : l_trig_val);
					positive_transition = 1;
				}
			}

			if (((sensor_value < u_reset_val) || (fabs(u_reset_val) == DBL_MAX))
			    && ((sensor_value > l_reset_val) || (fabs(l_reset_val) == DBL_MAX))) {
				CLEAR_BIT(ctx.s_state.tripped, sens_num);
				if (BIT_SET(ctx.s_state.last_state, sens_num)) {
					dlog("pacd[%d]: sensor %d (%s) returned to "
					     "normal range (%f).\n",
					     ctx.c->PAC_index, sens_num,
					     ctx.sensor_names[sens_num],
					     sensor_value);
					negative_transition = 1;
				}
			}

			if (BIT_SET(ctx.s_state.last_state, sens_num)
			    && BIT_SET(ctx.s_state.tripped, sens_num)) {
				dlog("pacd[%d]: sensor %d (%s) still tripped - "
				     "value is (%f).\n",
				     ctx.c->PAC_index, sens_num,
				     ctx.sensor_names[sens_num], sensor_value);
			}
		}

		bmcDestroySensorValues(&ctx.values);

		if (positive_transition) {
			must_PR = !ctx.c->has_been_PRd;
		} else if (negative_transition) {
			// Check for no remaining tripped sensors
			for (i = 0; i < num_sensors; i++) {
				if (BIT_SET(ctx.s_state.tripped, i)) {
					break;
				}
			}

			if (i == num_sensors) { // No remaining tripped sensors
				must_PR = 0;
				ctx.c->has_been_PRd = 0;
			}
		}

		if (must_PR) {
			ctx.c->has_been_PRd = 1;
			/* program NULL bitstream */
			dlog("pacd[%d]: writing default bitstream.\n",
			     ctx.c->PAC_index);

			if (ctx.c->config->remove_driver) {
				int tries = 100;
				sysfs_write_1(ctx.fme_token, "../device/reset");
				sysfs_write_1(ctx.fme_token,
					      "../device/remove");
				while ((res = pacd_bmc_shutdown(&ctx))
				       != FPGA_OK) {
					usleep(1000 * 1000);
				}
				clock_nanosleep(CLOCK_MONOTONIC, 0,
						&ctx.c->config->cooldown_delay,
						NULL);

				sysfs_write_1(NULL, "/sys/bus/pci/rescan");

				while ((res = pacd_bmc_reinit(&ctx))
				       != FPGA_OK) {
					usleep(1000 * 1000);
				}

				while (((res = fpgaOpen(ctx.fme_token,
							&fme_handle, 0))
					!= FPGA_OK)
				       && (tries >= 0)) {
					if (0 == (tries % 20)) {
						dlog("pacd[%d]: waiting for pci rescan.\n",
						     ctx.c->PAC_index);
					}
					tries--;
					usleep(500 * 1000);
				}

				if (FPGA_OK != res) {
					dlog("pacd[%d]: PANIC: driver not reloaded.\n",
					     ctx.c->PAC_index);
				}
			} else {

				res = fpgaOpen(ctx.fme_token, &fme_handle, 0);
				if (res != FPGA_OK) {
					dlog("pacd[%d]: failed to open FPGA.\n",
					     ctx.c->PAC_index);
					/* TODO: retry? */
					continue;
				}
			}

			res = fpgaReconfigureSlot(
				fme_handle, 0, ctx.null_gbs_info.data,
				ctx.null_gbs_info.data_len, FPGA_RECONF_FORCE);
			if (res != FPGA_OK) {
				dlog("pacd[%d]: failed to write bitstream.\n",
				     ctx.c->PAC_index);
				/* TODO: retry? */
			}

			res = fpgaClose(fme_handle);
			if (res != FPGA_OK) {
				dlog("pacd[%d]: failed to close FPGA.\n",
				     ctx.c->PAC_index);
			}
		}

		for (i = 0; i < num_sensors; i++) {
			if (BIT_SET(ctx.s_state.tripped, i)) {
				SET_BIT(ctx.s_state.last_state, i);
			} else {
				CLEAR_BIT(ctx.s_state.last_state, i);
			}
		}

		clock_nanosleep(CLOCK_MONOTONIC, 0,
				&ctx.c->config->poll_interval, NULL);
	}

out_exit:
	pacd_bmc_shutdown(&ctx);

	if (ctx.s_state.last_state) {
		free(ctx.s_state.last_state);
	}
	if (ctx.s_state.tripped) {
		free(ctx.s_state.tripped);
	}

	if (ctx.values) {
		bmcDestroySensorValues(&ctx.values);
	}

	if (ctx.records) {
		bmcDestroySDRs(&ctx.records);
	}

	return NULL;

out_destroy_values:
	bmcDestroySensorValues(&ctx.values);

	bmcDestroySDRs(&ctx.records);
	goto out_exit;
}

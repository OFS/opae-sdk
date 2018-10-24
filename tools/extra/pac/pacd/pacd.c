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
 * pacd: FPGA system daemon
 *
 * System deamon to
 *
 * - monitor FPGA status (errors, power, thermal)
 * - relay error/power/thermal events to FPGA API applications
 */

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include "opae/fpga.h"
#include "bmc_thermal.h"
#include "config_int.h"
#include "log.h"
#include "enumerate.h"
#include <getopt.h>
#include <stdlib.h>
#include <float.h>

#include "safe_string/safe_string.h"

int daemonize(void (*hndlr)(int, siginfo_t *, void *), mode_t, const char *);

#define UNUSED_PARAM(x) ((void)x)

#define PACD_LOCKFILE "/tmp/pacd.lock"

static struct timespec wait_for_card_ts = {
	.tv_sec = PACD_WAIT_FOR_CARD,
	.tv_nsec = 0,
};

#define OPT_STR ":hdP:l:p:m:S:B:D:F:n:t:T:i:c:N"

struct option longopts[] = {
	{"help", no_argument, NULL, 'h'},
	{"daemon", no_argument, NULL, 'd'},
	{"directory", required_argument, NULL, 'P'},
	{"logfile", required_argument, NULL, 'l'},
	{"pidfile", required_argument, NULL, 'p'},
	{"umask", required_argument, NULL, 'm'},
	{"segment", required_argument, NULL, 'S'},
	{"bus", required_argument, NULL, 'B'},
	{"device", required_argument, NULL, 'D'},
	{"function", required_argument, NULL, 'F'},
	{"default-bitstream", required_argument, NULL, 'n'},
	{"upper-sensor-threshold", required_argument, NULL, 'T'},
	{"lower-sensor-threshold", required_argument, NULL, 't'},
	{"cooldown-interval", required_argument, NULL, 'c'},
	{"poll-interval", required_argument, NULL, 'i'},
	{"no-defaults", no_argument, NULL, 'N'},
	{"driver-removal-disable", no_argument, NULL, 'r'}, // Hidden?
	{0, 0, 0, 0},
};

void show_help(void)
{
	FILE *fp = stdout;

	fprintf(fp, "Usage: pacd <options>\n");
	fprintf(fp, "\n");
	fprintf(fp, "\t-d,--daemon                   run as daemon process.\n");
	fprintf(fp,
		"\t-P,--directory <dir>          the working path for daemon mode [/tmp].\n");
	fprintf(fp,
		"\t-l,--logfile <file>           the log file for daemon mode [/tmp/pacd.log].\n");
	fprintf(fp,
		"\t-p,--pidfile <file>           the pid file for daemon mode [/tmp/pacd.pid].\n");
	fprintf(fp,
		"\t-m,--umask <mode>             the file mode for daemon mode [0].\n");
	fprintf(fp,
		"\t-i,--poll-interval <sec>      Time (in seconds) between sensor polling [5.0].\n");
	fprintf(fp,
		"\t-c,--cooldown-interval <sec>  Wait time for card cooldown [180.0].\n");
	fprintf(fp,
		"\t-S,--segment <seg>            the PCIe segment (domain) of the PAC.\n");
	fprintf(fp,
		"\t-B,--bus <bus>                the PCIe bus of the PAC.\n");
	fprintf(fp,
		"\t-D,--device <dev>             the PCIe device of the PAC.\n");
	fprintf(fp,
		"\t-F,--function <func>          the PCIe function of the PAC.\n");
	fprintf(fp,
		"\t    If no PCIe address elements are specified, run on all available PACs.\n");
	fprintf(fp,
		"\t-n,--default-bitstream <file> Default bitstream (for thermal shutdown handling,\n"
		"\t                              may be given multiple times).\n");
	fprintf(fp,
		"\t-T,--upper-sensor-threshold <sensor>:<trigger_thresh>[:<reset_thresh>]\n"
		"\t                              specify the threshold value of sensor <sensor>\n"
		"\t                              which will cause a trigger above <trigger_thresh>.\n"
		"\t                              The sensor will reset triggered state at <reset_thresh>.\n");
	fprintf(fp,
		"\t-t,--lower-sensor-threshold <sensor>:<trigger_thresh>[:<reset_thresh>]\n"
		"\t                              specify the threshold value of sensor <sensor>\n"
		"\t                              which will cause a trigger below <trigger_thresh>.\n"
		"\t                              The sensor will reset triggered state at <reset_thresh>.\n");
	fprintf(fp,
		"\t-N,--no-defaults              Do not add default sensor trip thresholds.\n"
		"\t                              All sensors to monitor given on command-line.\n"
		"\t                              Requires at least 1 -t or -T.  Default is add defaults.\n");
	fprintf(fp,
		"\t\n--driver-removal-disable      Advanced. Do not reset and remove FPGA device\n"
		"\t                              driver during cooldown. Load default GBS immediately.\n"
		"\t                              Default 0 (remove driver during cooldown).\n");
}

struct config config = {
	.verbosity = 0,
	.poll_interval =
		{
			.tv_sec = 0,
			.tv_nsec = 0,
		}, // 5-second interval
	.cooldown_delay =
		{
			.tv_sec = 180,
			.tv_nsec = 0,
		}, // 3-minute interval
	.daemon = 0,
	.directory = "/tmp",
	.logfile = "/tmp/pacd.log",
	.pidfile = "/tmp/pacd.pid",
	.filemode = 0,
	.running = true,
	.null_gbs = {0},
	.num_null_gbs = 0,
	.num_PACs = 0,
	.no_defaults = 0,
	.remove_driver = 1,
};

void sig_handler(int sig, siginfo_t *info, void *unused)
{
	UNUSED_PARAM(info);
	UNUSED_PARAM(unused);
	switch (sig) {
	case SIGINT:
		// Process terminated.
		dlog("Got SIGINT. Exiting.\n");
		config.running = false;
		break;
	case SIGTERM:
		// Process terminated.
		dlog("Got SIGTERM. Exiting.\n");
		config.running = false;
		break;
	}
}

int main(int argc, char *argv[])
{
	int getopt_ret;
	int option_index;
	int res;
	int i;
	int j;
	int result = 0;
	void *thread_result = NULL;

	pthread_t bmc_thermal[MAX_PAC_DEVICES]; /* one per device */
	struct bmc_thermal_context context[MAX_PAC_DEVICES];
	context[0].num_thresholds = 0;

	FILE *fp = fopen(PACD_LOCKFILE, "r");
	if (NULL != fp) {
		pid_t pid;
		int kret = 0;
		if (fscanf(fp, "%d", &pid) == 1) {
			errno = 0;
			kret = kill(pid, 0);
		}

		if ((0 == kret) || (EPERM == errno)) {
			fprintf(stderr,
				"%s: Already running, or invalid permissions\n",
				argv[0]);
			fclose(fp);
			return 1;
		}

		fclose(fp);
		fp = NULL;
		remove(PACD_LOCKFILE);
	}

	pthread_mutex_init(&config.reload_mtx, NULL);

	for (i = 0; i < MAX_SENSORS_TO_MONITOR; i++) {
		context[0].sensor_number[i] = -1;
		context[0].upper_trigger_value[i] = DBL_MAX;
		context[0].upper_reset_value[i] = -DBL_MAX;
		context[0].lower_trigger_value[i] = -DBL_MAX;
		context[0].lower_reset_value[i] = DBL_MAX;
	}

	context[0].segment = -1;
	context[0].bus = -1;
	context[0].device = -1;
	context[0].function = -1;

	fLog = stdout;

	while (-1
	       != (getopt_ret = getopt_long(argc, argv, OPT_STR, longopts,
					    &option_index))) {
		const char *tmp_optarg = optarg;

		if (optarg && ('=' == *tmp_optarg))
			++tmp_optarg;

		if (!optarg && (NULL != argv[optind])
		    && ('-' != argv[optind][0]))
			tmp_optarg = argv[optind++];

		switch (getopt_ret) {
		case 'h':
			show_help();
			return 0;
			break;

		case 'd':
			config.daemon = 1;
			dlog("daemon requested\n");
			break;

		case 'r':
			config.remove_driver = 0;
			dlog("driver removal disabled\n");
			break;

		case 'P':
			if (tmp_optarg) {
				config.directory = tmp_optarg;
				dlog("daemon path is %s\n", config.directory);
			} else {
				fprintf(stderr, "missing path parameter.\n");
				return 1;
			}
			break;

		case 'l':
			if (tmp_optarg) {
				config.logfile = tmp_optarg;
				dlog("daemon log file is %s\n", config.logfile);
			} else {
				fprintf(stderr, "missing logfile parameter.\n");
				return 1;
			}
			break;

		case 'p':
			if (tmp_optarg) {
				config.pidfile = tmp_optarg;
				dlog("daemon pid file is %s\n", config.pidfile);
			} else {
				fprintf(stderr, "missing pidfile parameter.\n");
				return 1;
			}
			break;

		case 'm':
			if (tmp_optarg) {
				config.filemode =
					(mode_t)strtoul(tmp_optarg, NULL, 0);
				dlog("daemon umask is 0x%x\n", config.filemode);
			} else {
				fprintf(stderr,
					"missing filemode parameter.\n");
				return 1;
			}
			break;

		case 'i': // double seconds between polls
			if (!tmp_optarg) {
				fprintf(stderr, "invalid poll interval.\n");
				return 1;
			}

			char *endptr1 = NULL;
			errno = 0;
			double val1 = strtod(tmp_optarg, &endptr1);

			if (errno || ((0 == val1) && (endptr1 == tmp_optarg))
			    || (val1 < 0)) {
				fprintf(stderr, "invalid poll interval.\n");
				return 1;
			}

			config.poll_interval.tv_sec = (time_t)val1;
			config.poll_interval.tv_nsec =
				(long)((val1 - (time_t)val1) * 1e9);
			dlog("Polling interval set to %f sec\n", val1);
			break;

		case 'c': // double cooldown seconds
			if (!tmp_optarg) {
				fprintf(stderr,
					"invalid cooldown delay time.\n");
				return 1;
			}

			endptr1 = NULL;
			errno = 0;
			val1 = strtod(tmp_optarg, &endptr1);

			if (errno || ((0 == val1) && (endptr1 == tmp_optarg))
			    || (val1 < 0) || (val1 > 3600.0)) {
				fprintf(stderr,
					"invalid cooldown delay time.\n");
				return 1;
			}

			config.cooldown_delay.tv_sec = (time_t)val1;
			config.cooldown_delay.tv_nsec =
				(long)((val1 - (time_t)val1) * 1e9);
			dlog("Cooldown delay set to %f sec\n", val1);
			break;

		case 'n':
			if (tmp_optarg) {
				if (config.num_null_gbs < MAX_NULL_GBS) {
					config.null_gbs[config.num_null_gbs++] =
						tmp_optarg;
					dlog("registering default bitstream \"%s\"\n",
					     tmp_optarg);
					/* TODO: check NULL bitstream
					 * compatibility */
				} else {
					dlog("maximum number of default bitstreams exceeded, ignoring -n option\n");
				}
			} else {
				fprintf(stderr,
					"missing bitstream parameter.\n");
				return 1;
			}
			break;

		case 'T': // <sensor>:<trigger_thresh>[:<reset_thresh>] - Upper
		{
			if (!tmp_optarg) {
				fprintf(stderr,
					"invalid threshold parameter.\n");
				return 1;
			}

			char *colon = strchr(tmp_optarg, ':');
			if ((NULL == colon) || (*tmp_optarg == ':')) {
				fprintf(stderr,
					"invalid threshold parameter.\n");
				return 1;
			}
			*colon++ = '\0';
			endptr1 = NULL;
			char *endptr2 = NULL;
			char *endptr3 = NULL;
			double val3 = 0;
			errno = 0;
			unsigned long lval1 = strtoul(tmp_optarg, &endptr1, 0);
			int err1 = errno;
			errno = 0;
			double val2 = strtod(colon, &endptr2);
			int err2 = errno;
			errno = 0;
			int err3 = 0;

			char *nxt_colon = strchr(colon, ':');
			// Force endptr3 to be different for validity check
			// below
			endptr3 = (char *)(~(uint64_t)nxt_colon);
			if (NULL != nxt_colon) {
				val3 = strtod(++nxt_colon, &endptr3);
				err3 = errno;
			} else {
				val3 = val2;
			}

			if (err1 || err2 || err3
			    || ((0 == lval1) && (endptr1 == tmp_optarg))
			    || ((0 == val2) && (endptr2 == colon))
			    || ((0 == val3) && (endptr3 == nxt_colon))
			    || ((long)lval1 < 0) || (val3 > val2)) {
				fprintf(stderr,
					"invalid upper threshold parameter for sensor %ld.\n",
					lval1);
				return 1;
			}

			if (context[0].num_thresholds
			    >= MAX_SENSORS_TO_MONITOR) {
				dlog("Max thresholds exceeded.\n");
				return 1;
			}

			unsigned int ii;
			int set_thresh = 0;
			int thresh_ndx = -1;
			for (ii = 0; ii < context[0].num_thresholds; ii++) {
				int32_t snum = context[0].sensor_number[ii];
				if ((int32_t)lval1 == snum) {
					thresh_ndx = ii;
					if (context[0].upper_trigger_value[ii]
					    != DBL_MAX) {
						dlog("WARNING: Multiple upper threshold values for "
						     "sensor %d - using %f : %f.\n",
						     snum, val2, val3);
						context[0].upper_trigger_value
							[ii] = val2;
						context[0]
							.upper_reset_value[ii] =
							val3;
						set_thresh = 1;
					}
					break;
				}
			}

			if (!set_thresh) {
				if (thresh_ndx == -1) {
					context[0].sensor_number
						[context[0].num_thresholds] =
						lval1;
					context[0].upper_trigger_value
						[context[0].num_thresholds] =
						val2;
					context[0].upper_reset_value
						[context[0].num_thresholds] =
						val3;
					context[0].num_thresholds++;
				} else {
					context[0].upper_trigger_value
						[thresh_ndx] = val2;
					context[0]
						.upper_reset_value[thresh_ndx] =
						val3;
				}
				dlog("sensor %d upper thresholds set to %f (trip) %f (reset)\n",
				     lval1, val2, val3);
			}
			break;
		}

		case 't': // <sensor>:<trigger_thresh>[:<reset_thresh>] - Lower
		{
			if (!tmp_optarg) {
				fprintf(stderr,
					"invalid threshold parameter.\n");
				return 1;
			}

			char *colon = strchr(tmp_optarg, ':');
			if ((NULL == colon) || (*tmp_optarg == ':')) {
				fprintf(stderr,
					"invalid threshold parameter.\n");
				return 1;
			}
			*colon++ = '\0';
			endptr1 = NULL;
			char *endptr2 = NULL;
			char *endptr3 = NULL;
			double val3 = 0;
			errno = 0;
			unsigned long lval1 = strtoul(tmp_optarg, &endptr1, 0);
			int err1 = errno;
			errno = 0;
			double val2 = strtod(colon, &endptr2);
			int err2 = errno;
			errno = 0;
			int err3 = 0;

			char *nxt_colon = strchr(colon, ':');
			// Force endptr3 to be different for validity check
			// below
			endptr3 = (char *)(~(uint64_t)nxt_colon);
			if (NULL != nxt_colon) {
				val3 = strtod(++nxt_colon, &endptr3);
				err3 = errno;
			} else {
				val3 = val2;
			}

			if (err1 || err2 || err3
			    || ((0 == lval1) && (endptr1 == tmp_optarg))
			    || ((0 == val2) && (endptr2 == colon))
			    || ((0 == val3) && (endptr3 == nxt_colon))
			    || ((long)lval1 < 0) || (val3 < val2)) {
				fprintf(stderr,
					"invalid lower threshold parameter for sensor %ld.\n",
					lval1);
				return 1;
			}

			if (context[0].num_thresholds
			    >= MAX_SENSORS_TO_MONITOR) {
				dlog("Max thresholds exceeded.\n");
				return 1;
			}

			unsigned int ii;
			int set_thresh = 0;
			int thresh_ndx = -1;
			for (ii = 0; ii < context[0].num_thresholds; ii++) {
				int32_t snum = context[0].sensor_number[ii];
				if ((int32_t)lval1 == snum) {
					thresh_ndx = ii;
					if (context[0].lower_trigger_value[ii]
					    != -DBL_MAX) {
						dlog("WARNING: Multiple lower threshold values for "
						     "sensor %d - using %f : %f.\n",
						     snum, val2, val3);
						context[0].lower_trigger_value
							[ii] = val2;
						context[0]
							.lower_reset_value[ii] =
							val3;
						set_thresh = 1;
					}
					break;
				}
			}

			if (!set_thresh) {
				if (thresh_ndx == -1) {
					context[0].sensor_number
						[context[0].num_thresholds] =
						lval1;
					context[0].lower_trigger_value
						[context[0].num_thresholds] =
						val2;
					context[0].lower_reset_value
						[context[0].num_thresholds] =
						val3;
					context[0].num_thresholds++;
				} else {
					context[0].lower_trigger_value
						[thresh_ndx] = val2;
					context[0]
						.lower_reset_value[thresh_ndx] =
						val3;
				}
				dlog("sensor %ld lower thresholds set to %f (trip) %f (reset)\n",
				     lval1, val2, val3);
			}
			break;
		}

		case 'S':
			if (tmp_optarg) {
				context[0].segment =
					strtoul(tmp_optarg, NULL, 0);
				dlog("PAC PCIe segment is 0x%04x\n",
				     context[0].segment);
			} else {
				fprintf(stderr, "missing segment parameter.\n");
				return 1;
			}
			break;

		case 'B':
			if (tmp_optarg) {
				context[0].bus = strtoul(tmp_optarg, NULL, 0);
				dlog("PAC PCIe bus is 0x%02x\n",
				     context[0].bus);
			} else {
				fprintf(stderr, "missing bus parameter.\n");
				return 1;
			}
			break;

		case 'D':
			if (tmp_optarg) {
				context[0].device =
					strtoul(tmp_optarg, NULL, 0);
				dlog("PAC PCIe device is 0x%02x\n",
				     context[0].device);
			} else {
				fprintf(stderr, "missing device parameter.\n");
				return 1;
			}
			break;

		case 'F':
			if (tmp_optarg) {
				context[0].function =
					strtoul(tmp_optarg, NULL, 0);
				dlog("PAC PCIe function is 0x%1x\n",
				     context[0].function);
			} else {
				fprintf(stderr,
					"missing function parameter.\n");
				return 1;
			}
			break;

		case 'N':
			config.no_defaults = 1;
			dlog("only monitoring specified sensors.\n");
			break;

		case ':':
			dlog("Missing option argument.\n");
			return 1;
			break;

		case '?':
		default:
			dlog("Invalid command option.\n");
			return 1;
			break;
		}
	}

	if ((1 == config.no_defaults) && (0 == context[0].num_thresholds)) {
		dlog("ERROR: No sensor thresholds specified and no-defaults option specified.\n");
		return 1;
	}

	// Enumerate all the PAC devices, then set up monitoring threads
	// for each
	int num_PACs = 0;
	int initial_msg = 0;
	context[0].config = &config;
	do {
		num_PACs = enumerate(&context[0]);

		if ((num_PACs < 1) && (!config.daemon)) {
			fprintf(stderr, "No PAC cards found.\n");
			return 1;
		}
		if (num_PACs < 1) {
			if (!initial_msg) {
				initial_msg = 1;
				dlog("Waiting for PAC card / driver\n");
			}
			clock_nanosleep(CLOCK_MONOTONIC, 0, &wait_for_card_ts,
					NULL);
			initial_msg++;
		}
	} while ((num_PACs < 1) && (initial_msg < 1000));

	if (initial_msg >= 1000) {
		dlog("PAC card not found - terminating");
		result = -1;
		goto out;
	}

	if (initial_msg) {
		dlog("PAC card / driver found - continuing");
	}

	if (num_PACs > MAX_PAC_DEVICES) {
		dlog("WARNING: %d PACs found, will only monitor %d.\n",
		     num_PACs, MAX_PAC_DEVICES);
		num_PACs = MAX_PAC_DEVICES;
	}

	if (config.daemon) {
		FILE *fp;

		/* daemonize also registers sig_handler for SIGINT */
		res = daemonize(sig_handler, config.filemode, config.directory);
		if (res != 0) {
			dlog("daemonize failed: %s\n", strerror(res));
			return 1;
		}

		fp = fopen(config.pidfile, "w");
		if (NULL == fp) {
			return 1;
		}
		fprintf(fp, "%d\n", getpid());
		fclose(fp);

		if (open_log(config.logfile) != 0)
			fprintf(stderr, "failed to open logfile\n");


	} else {
		struct sigaction sa;

		memset_s(&sa, sizeof(sa), 0);
		sa.sa_flags = SA_SIGINFO | SA_RESETHAND;
		sa.sa_sigaction = sig_handler;

		res = sigaction(SIGINT, &sa, NULL);
		if (res < 0) {
			dlog("failed to register signal handler.\n");
			return 1;
		}

		res = sigaction(SIGTERM, &sa, NULL);
		if (res < 0) {
			dlog("failed to register signal handler.\n");
			return 1;
		}
	}

	fp = fopen(PACD_LOCKFILE, "w");
	if (NULL == fp) {
		return 1;
	}
	fprintf(fp, "%d\n", getpid());
	fclose(fp);

	for (i = 1; i < num_PACs; i++) {
		memcpy_s(&context[i], sizeof(context[i]), &context[0],
			 sizeof(context[0]));
	}

	for (i = 0; i < num_PACs; i++) {
		context[i].config = &config;
		context[i].PAC_index = i;
		context[i].has_been_PRd = 0;
		context[i].fme_token = config.tokens[i];
		res = pthread_create(&bmc_thermal[i], NULL, bmc_thermal_thread,
				     &context[i]);
		if (res) {
			dlog("failed to create BMC_THERMAL thread #%i.\n", i);
			config.running = false;
			for (j = 0; j < i; j++) {
				pthread_join(bmc_thermal[j], NULL);
			}
			return 1;
		}
	}

	for (i = 0; i < num_PACs; i++) {
		pthread_join(bmc_thermal[i], &thread_result);
		result |= (thread_result != NULL);
	}

	if (stdout != fLog) {
		close_log();
	}

	if (config.daemon) {
		remove(config.pidfile);
	}

out:
	remove(PACD_LOCKFILE);

	if (pthread_mutex_destroy(&config.reload_mtx)) {
		dlog("pthread_mutex_destroy failed\n");
	}

	return result;
}

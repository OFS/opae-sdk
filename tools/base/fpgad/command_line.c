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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <getopt.h>
#include "command_line.h"
#include "config_file.h"
#include "monitored_device.h"

#ifdef LOG
#undef LOG
#endif
#define LOG(format, ...) \
log_printf("args: " format, ##__VA_ARGS__)

extern fpgad_supported_device default_supported_devices_table[];

#define OPT_STR ":hdl:p:s:n:c:"

STATIC struct option longopts[] = {
	{ "help",           no_argument,       NULL, 'h' },
	{ "daemon",         no_argument,       NULL, 'd' },
	{ "logfile",        required_argument, NULL, 'l' },
	{ "pidfile",        required_argument, NULL, 'p' },
	{ "socket",         required_argument, NULL, 's' },
	{ "null-bitstream", required_argument, NULL, 'n' },
	{ "config",         required_argument, NULL, 'c' },

	{ 0, 0, 0, 0 }
};

#define DEFAULT_DIR_ROOT      "/var/lib/opae"
#define DEFAULT_DIR_ROOT_SIZE 13
#define DEFAULT_LOG           "fpgad.log"
#define DEFAULT_PID           "fpgad.pid"
#define DEFAULT_CFG           "fpgad.cfg"

void cmd_show_help(FILE *fptr)
{
	fprintf(fptr, "Usage: fpgad <options>\n");
	fprintf(fptr, "\n");
	fprintf(fptr, "\t-d,--daemon                 run as daemon process.\n");
	fprintf(fptr, "\t-l,--logfile <file>         the log file for daemon mode [%s].\n", DEFAULT_LOG);
	fprintf(fptr, "\t-p,--pidfile <file>         the pid file for daemon mode [%s].\n", DEFAULT_PID);
	fprintf(fptr, "\t-s,--socket <sock>          the unix domain socket [/tmp/fpga_event_socket].\n");
	fprintf(fptr, "\t-n,--null-bitstream <file>  NULL bitstream (for AP6 handling, may be\n"
		      "\t                            given multiple times).\n");
	fprintf(fptr, "\t-c,--config <file>          the configuration file [%s].\n", DEFAULT_CFG);
}

STATIC bool cmd_register_null_gbs(struct fpgad_config *c, char *null_gbs_path)
{
	char *canon_path = NULL;

	if (c->num_null_gbs < (sizeof(c->null_gbs) / sizeof(c->null_gbs[0]))) {
		canon_path = canonicalize_file_name(null_gbs_path);

		if (canon_path) {

			if (opae_load_bitstream(canon_path,
						&c->null_gbs[c->num_null_gbs])) {
				LOG("failed to load NULL GBS \"%s\"\n", canon_path);
				free(canon_path);
				return false;
			}

			c->num_null_gbs++;

			LOG("registering NULL bitstream \"%s\"\n", canon_path);

		} else {
			LOG("error with NULL GBS argument: %s\n", strerror(errno));
			return false;
		}

	} else {
		LOG("maximum number of NULL bitstreams exceeded. Ignoring -n option.\n");
	}
	return true;
}

int cmd_parse_args(struct fpgad_config *c, int argc, char *argv[])
{
	int getopt_ret;
	int option_index;

	while (-1 != (getopt_ret = getopt_long(argc, argv, OPT_STR, longopts, &option_index))) {
		const char *tmp_optarg = optarg;

		if (optarg && ('=' == *tmp_optarg))
			++tmp_optarg;

		if (!optarg && (optind < argc) &&
			(NULL != argv[optind]) &&
			('-' != argv[optind][0]))
			tmp_optarg = argv[optind++];

		switch (getopt_ret) {
		case 'h':
			cmd_show_help(stdout);
			return -2;
			break;

		case 'd':
			c->daemon = 1;
			LOG("daemon requested\n");
			break;

		case 'l':
			if (tmp_optarg) {
				strncpy_s(c->logfile, sizeof(c->logfile),
						tmp_optarg, PATH_MAX);
			} else {
				LOG("missing logfile parameter.\n");
				return 1;
			}
			break;

		case 'p':
			if (tmp_optarg) {
				strncpy_s(c->pidfile, sizeof(c->pidfile),
						tmp_optarg, PATH_MAX);
			} else {
				LOG("missing pidfile parameter.\n");
				return 1;
			}
			break;

		case 'n':
			if (tmp_optarg) {
				if (!cmd_register_null_gbs(c, (char *)tmp_optarg)) {
					LOG("invalid null gbs path: \"%s\"\n", tmp_optarg);
					return 1;
				}
			} else {
				LOG("missing bitstream parameter.\n");
				return 1;
			}
			break;

		case 's':
			if (tmp_optarg) {
				c->api_socket = tmp_optarg;
				LOG("daemon socket is %s\n", c->api_socket);
			} else {
				LOG("missing socket parameter.\n");
				return 1;
			}
			break;

		case 'c':
			if (tmp_optarg) {
				strncpy_s(c->cfgfile, sizeof(c->cfgfile),
						tmp_optarg, PATH_MAX);
			} else {
				LOG("missing cfgfile parameter.\n");
				return 1;
			}
			break;

		case ':':
			LOG("Missing option argument.\n");
			return 1;

		case '?':
			LOG("Invalid command option.\n");
			return 1;

		default:
			LOG("Invalid command option.\n");
			return 1;
		}

	}

	return 0;
}

void cmd_canonicalize_paths(struct fpgad_config *c)
{
	char *sub;
	bool def;
	mode_t mode;
	struct stat stat_buf;
	bool search = true;

	if (!geteuid()) {
		// If we're being run as root, then use DEFAULT_DIR_ROOT
		// as the working directory.
		strncpy_s(c->directory, sizeof(c->directory),
				DEFAULT_DIR_ROOT, DEFAULT_DIR_ROOT_SIZE);
		mode = 0755;
		c->filemode = 0026;
	} else {
		// We're not root. Try to use ${HOME}/.opae
		char *home = getenv("HOME");
		char *canon_path = NULL;
		int res = 1;

		// Accept ${HOME} only if it is rooted at /home.
		if (home) {
			canon_path = canonicalize_file_name(home);
			int len = strnlen_s(canon_path, PATH_MAX);
			if (len >= 5)
				strcmp_s(canon_path, 5, "/home/", &res);
		}

		if (canon_path && !res) {
			snprintf_s_s(c->directory, sizeof(c->directory),
					"%s/.opae", canon_path);
		} else {
			char cwd[PATH_MAX];
			// ${HOME} not found or invalid - use current dir.
			if (getcwd(cwd, sizeof(cwd))) {
				snprintf_s_s(c->directory, sizeof(c->directory),
						"%s/.opae", cwd);
			} else {
				// Current directory not found - use /
				strncpy_s(c->directory, sizeof(c->directory),
						"/.opae", 6);
			}
		}

		if (canon_path) {
			free(canon_path);
		}
		mode = 0775;
		c->filemode = 0022;
	}
	LOG("daemon working directory is %s\n", c->directory);

	// Create the directory if it doesn't exist.
	if (lstat(c->directory, &stat_buf) && (errno == ENOENT)) {
		if (mkdir(c->directory, mode))
			LOG("mkdir failed\n");
	}

	// Verify logfile and pidfile do not contain ".."
	// nor "/".
	def = false;
	sub = NULL;
	strstr_s(c->logfile, sizeof(c->logfile),
			"..", 2, &sub);
	if (sub)
		def = true;

	sub = NULL;
	strstr_s(c->logfile, sizeof(c->logfile),
			"/", 1, &sub);
	if (sub)
		def = true;

	if (def || (c->logfile[0] == '\0')) {
		snprintf_s_ss(c->logfile, sizeof(c->logfile),
				"%s/%s", c->directory, DEFAULT_LOG);
	} else {
		char tmp[PATH_MAX];
		strncpy_s(tmp, sizeof(tmp),
				c->logfile, sizeof(c->logfile));
		snprintf_s_ss(c->logfile, sizeof(c->logfile),
				"%s/%s", c->directory, tmp);
	}
	LOG("daemon log file is %s\n", c->logfile);

	def = false;
	sub = NULL;
	strstr_s(c->pidfile, sizeof(c->pidfile),
			"..", 2, &sub);
	if (sub)
		def = true;

	sub = NULL;
	strstr_s(c->pidfile, sizeof(c->pidfile),
			"/", 1, &sub);
	if (sub)
		def = true;

	if (def || (c->pidfile[0] == '\0')) {
		snprintf_s_ss(c->pidfile, sizeof(c->pidfile),
				"%s/%s", c->directory, DEFAULT_PID);
	} else {
		char tmp[PATH_MAX];
		strncpy_s(tmp, sizeof(tmp),
				c->pidfile, sizeof(c->pidfile));
		snprintf_s_ss(c->pidfile, sizeof(c->pidfile),
				"%s/%s", c->directory, tmp);
	}
	LOG("daemon pid file is %s\n", c->pidfile);

	// Verify cfgfile doesn't contain ".."
	def = false;
	sub = NULL;
	strstr_s(c->cfgfile, sizeof(c->cfgfile),
			"..", 2, &sub);
	if (sub)
		def = true;

	if (def || (c->cfgfile[0] == '\0')) {
		search = true;
	} else {
		char *canon_path;

		canon_path = canonicalize_file_name(c->cfgfile);
		if (canon_path) {

			stat(c->cfgfile, &stat_buf);

			// prevent symlinks
			if (!S_ISLNK(stat_buf.st_mode)) {

				strncpy_s(c->cfgfile,
					  sizeof(c->cfgfile),
					  canon_path,
					  strnlen_s(canon_path, PATH_MAX));

				if (!cfg_load_config(c)) {
					LOG("daemon cfg file is %s\n",
					    c->cfgfile);
					search = false; // found and loaded it
				}

			}

			free(canon_path);
		}
	}

	if (search) {
		c->cfgfile[0] = '\0';
		if (cfg_find_config_file(c))
			LOG("failed to find config file.\n");
		else {
			if (cfg_load_config(c))
				LOG("failed to load config file %s\n",
				    c->cfgfile);
			else
				LOG("daemon cfg file is %s\n", c->cfgfile);
		}
	}

	if (!c->supported_devices) {
		LOG("using default configuration.\n");
		c->cfgfile[0] = '\0';
		c->supported_devices = default_supported_devices_table;
	}
}

void cmd_destroy(struct fpgad_config *c)
{
	unsigned i;

	if (c->daemon)
		unlink(c->pidfile);

	for (i = 0 ; i < c->num_null_gbs ; ++i) {
		if (c->null_gbs[i].filename)
			free((char *)c->null_gbs[i].filename);
		opae_unload_bitstream(&c->null_gbs[i]);
	}
	c->num_null_gbs = 0;

	if (c->supported_devices &&
	    (c->supported_devices != default_supported_devices_table)) {
		free(c->supported_devices);
	}
	c->supported_devices = NULL;
}

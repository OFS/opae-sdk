// Copyright(c) 2018-2022, Intel Corporation
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

#define _GNU_SOURCE
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <getopt.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include "command_line.h"
#include "config_file.h"
#include "monitored_device.h"
#include "mock/opae_std.h"

#ifdef LOG
#undef LOG
#endif
#define LOG(format, ...) \
log_printf("args: " format, ##__VA_ARGS__)

extern fpgad_supported_device default_supported_devices_table[];

#define OPT_STR ":hdl:p:s:n:c:v"

STATIC struct option longopts[] = {
	{ "help",           no_argument,       NULL, 'h' },
	{ "daemon",         no_argument,       NULL, 'd' },
	{ "logfile",        required_argument, NULL, 'l' },
	{ "pidfile",        required_argument, NULL, 'p' },
	{ "socket",         required_argument, NULL, 's' },
	{ "null-bitstream", required_argument, NULL, 'n' },
	{ "config",         required_argument, NULL, 'c' },
	{ "version",        no_argument,       NULL, 'v' },

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
	fprintf(fptr, "\t-v,--version                display the version and exit.\n");
}

STATIC bool cmd_register_null_gbs(struct fpgad_config *c, char *null_gbs_path)
{
	char *canon_path = NULL;

	if (c->num_null_gbs < (sizeof(c->null_gbs) / sizeof(c->null_gbs[0]))) {
		canon_path = canonicalize_file_name(null_gbs_path);

		if (canon_path) {

			memset(&c->null_gbs[c->num_null_gbs], 0,
				 sizeof(opae_bitstream_info));

			if (opae_load_bitstream(canon_path,
						&c->null_gbs[c->num_null_gbs])) {
				LOG("failed to load NULL GBS \"%s\"\n", canon_path);
				opae_unload_bitstream(&c->null_gbs[c->num_null_gbs]);
				opae_free(canon_path);
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
	size_t len;

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
				len = strnlen(tmp_optarg, PATH_MAX - 1);
				memcpy(c->logfile, tmp_optarg, len);
				c->logfile[len] = '\0';
			} else {
				LOG("missing logfile parameter.\n");
				return 1;
			}
			break;

		case 'p':
			if (tmp_optarg) {
				len = strnlen(tmp_optarg, PATH_MAX - 1);
				memcpy(c->pidfile, tmp_optarg, len);
				c->pidfile[len] = '\0';
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
				len = strnlen(tmp_optarg, PATH_MAX - 1);
				memcpy(c->cfgfile, tmp_optarg, len);
				c->cfgfile[len] = '\0';
			} else {
				LOG("missing cfgfile parameter.\n");
				return 1;
			}
			break;

		case 'v':
			fprintf(stdout, "fpgad %s %s%s\n",
					OPAE_VERSION,
					OPAE_GIT_COMMIT_HASH,
					OPAE_GIT_SRC_TREE_DIRTY ? "*":"");
			return -2;
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

int cmd_canonicalize_paths(struct fpgad_config *c)
{
	char *sub;
	bool def;
	mode_t mode;
	struct stat stat_buf;
	bool search = true;
	char buf[PATH_MAX];
	char *canon_path;
	uid_t uid;
	size_t len;

	uid = geteuid();

	if (!uid) {
		// If we're being run as root, then use DEFAULT_DIR_ROOT
		// as the working directory.
		memcpy(c->directory, DEFAULT_DIR_ROOT, sizeof(DEFAULT_DIR_ROOT));
		c->directory[sizeof(DEFAULT_DIR_ROOT)] = '\0';
		mode = 0755;
		c->filemode = 0026;
	} else {
		// We're not root. Try to use ${HOME}/.opae
		struct passwd *passwd;

		passwd = getpwuid(uid);

		canon_path = canonicalize_file_name(passwd->pw_dir);

		if (canon_path) {
			snprintf(c->directory, sizeof(c->directory),
					"%s/.opae", canon_path);
			opae_free(canon_path);
		} else {
			// ${HOME} not found or invalid - use current dir.
			if (getcwd(buf, sizeof(buf))) {
				if (snprintf(c->directory, sizeof(c->directory),
					     "%s/.opae", buf) < 0) {
					len = strnlen("./.opae",
						sizeof(c->directory) - 1);
					memcpy(c->directory, "./.opae", len);
					c->directory[len] = '\0';
				}
			} else {
				// Current directory not found - use /
				len = strnlen("/.opae", sizeof(c->directory) - 1);
				memcpy(c->directory, "/.opae", len);
				c->directory[len] = '\0';
			}
		}

		mode = 0775;
		c->filemode = 0022;
	}

	if (cmd_path_is_symlink(c->directory)) {
		LOG("Aborting - working directory contains a link: %s\n.",
		    c->directory);
		return 1;
	}
	LOG("daemon working directory is %s\n", c->directory);

	// Create the directory if it doesn't exist.
	if (opae_lstat(c->directory, &stat_buf) && (errno == ENOENT)) {
		if (mkdir(c->directory, mode)) {
			LOG("mkdir failed\n");
			return 1;
		}
	}

	// Verify logfile and pidfile do not contain ".."
	// nor "/".
	def = false;
	sub = strstr(c->logfile, "..");
	if (sub)
		def = true;

	sub = strstr(c->logfile, "/");
	if (sub)
		def = true;

	if (def || (c->logfile[0] == '\0')) {
		if (snprintf(c->logfile, sizeof(c->logfile),
			     "%s/%s", c->directory, DEFAULT_LOG) < 0) {
			len = strnlen("./" DEFAULT_LOG,
					sizeof(c->logfile) - 1);
			memcpy(c->logfile, "./" DEFAULT_LOG, len);
			c->logfile[len] = '\0';
		}
	} else {
		len = strnlen(c->logfile, sizeof(buf) - 1);
		memcpy(buf, c->logfile, len);
		buf[len] = '\0';

		if (snprintf(c->logfile, sizeof(c->logfile),
			     "%s/%s", c->directory, buf) < 0) {
			len = strnlen("./" DEFAULT_LOG,
					sizeof(c->logfile) - 1);
			memcpy(c->logfile, "./" DEFAULT_LOG, len);
			c->logfile[len] = '\0';
		}
	}

	if (cmd_path_is_symlink(c->logfile)) {
		LOG("Aborting - log file path contains a link: %s\n.",
		    c->logfile);
		return 1;
	}
	LOG("daemon log file is %s\n", c->logfile);

	def = false;
	sub = strstr(c->pidfile, "..");
	if (sub)
		def = true;

	sub = strstr(c->pidfile, "/");
	if (sub)
		def = true;

	if (def || (c->pidfile[0] == '\0')) {

		if (snprintf(c->pidfile, sizeof(c->pidfile),
			     "%s/%s", c->directory, DEFAULT_PID) < 0) {
			len = strnlen("./" DEFAULT_PID,
					sizeof(c->pidfile) - 1);
			memcpy(c->pidfile, "./" DEFAULT_PID, len);
			c->pidfile[len] = '\0';
		}

	} else {
		len = strnlen(c->pidfile, sizeof(buf) - 1);
		memcpy(buf, c->pidfile, len);
		buf[len] = '\0';

		if (snprintf(c->pidfile, sizeof(c->pidfile),
			     "%s/%s", c->directory, buf) < 0) {
			len = strnlen("./" DEFAULT_PID,
					sizeof(c->pidfile) - 1);
			memcpy(c->pidfile, "./" DEFAULT_PID, len);
			c->pidfile[len] = '\0';
		}
	}

	if (cmd_path_is_symlink(c->pidfile)) {
		LOG("Aborting - pid file path contains a link: %s\n.",
		    c->pidfile);
		return 1;
	}
	LOG("daemon pid file is %s\n", c->pidfile);

	// Verify cfgfile doesn't contain ".."
	def = false;
	sub = strstr(c->cfgfile, "..");
	if (sub)
		def = true;

	if (def || (c->cfgfile[0] == '\0')) {
		search = true;
	} else {
		canon_path = canonicalize_file_name(c->cfgfile);
		if (canon_path) {

			if (!cmd_path_is_symlink(c->cfgfile)) {

				len = strnlen(canon_path,
					      sizeof(c->cfgfile) - 1);
				memcpy(c->cfgfile,
					  canon_path,
					  len);
				c->cfgfile[len] = '\0';

				if (!cfg_load_config(c)) {
					LOG("daemon cfg file is %s\n",
					    c->cfgfile);
					search = false; // found and loaded it
				}

			}

			opae_free(canon_path);
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

	return 0;
}

void cmd_destroy(struct fpgad_config *c)
{
	unsigned i;

	if (c->daemon)
		unlink(c->pidfile);

	for (i = 0 ; i < c->num_null_gbs ; ++i) {
		if (c->null_gbs[i].filename)
			opae_free((char *)c->null_gbs[i].filename);
		opae_unload_bitstream(&c->null_gbs[i]);
	}
	c->num_null_gbs = 0;

	if (c->supported_devices &&
	    (c->supported_devices != default_supported_devices_table)) {

		for (i = 0 ; c->supported_devices[i].library_path ; ++i) {
			fpgad_supported_device *d = &c->supported_devices[i];
			if (d->library_path)
				opae_free((void *)d->library_path);
			if (d->config)
				opae_free((void *)d->config);
		}

		opae_free(c->supported_devices);
	}
	c->supported_devices = NULL;
}

bool cmd_path_is_symlink(const char *path)
{
	char component[PATH_MAX];
	struct stat stat_buf;
	size_t len;
	char *pslash;

	len = strnlen(path, PATH_MAX - 1);
	if (!len) // empty path
		return false;

	memcpy(component, path, len);
	component[len] = '\0';

	if (component[0] == '/') {
		// absolute path

		pslash = opae_realpath(path, component);

		if (strcmp(component, path))
			return true;


	} else {
		// relative path

		pslash = strrchr(component, '/');

		while (pslash) {

			if (opae_fstatat(AT_FDCWD, component,
				    &stat_buf, AT_SYMLINK_NOFOLLOW)) {
				return false;
			}

			if (S_ISLNK(stat_buf.st_mode))
				return true;

			*pslash = '\0';
			pslash = strrchr(component, '/');
		}

		if (opae_fstatat(AT_FDCWD, component,
			    &stat_buf, AT_SYMLINK_NOFOLLOW)) {
			return false;
		}

		if (S_ISLNK(stat_buf.st_mode))
			return true;

	}

	return false;
}

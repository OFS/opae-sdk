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
// POSSIBILITY OF SUCH DAMAGE.

#include <getopt.h>
#include "fpgainfo.h"
#include "sysinfo.h"
#include "macinfo.h"
#include <opae/fpga.h>
#include <wchar.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include "safe_string/safe_string.h"

/*
 * Print help
 */
void mac_help(void)
{
	printf("\nPrint MAC information\n"
		   "        fpgainfo mac [-h]\n"
		   "                -h,--help           Print this help\n"
		   "\n");
}

#define I2C_DEVNAME  	"i2c"
#define MACROM_DEVNAME  "nvmem"
#define EEPROM_DEVNAME  "eeprom"

static int get_mac_rom_path(const char *in_path, const char *key_str,
							char *out_path, int size)
{
	DIR *dir = NULL;
	struct dirent *dirent = NULL;
	char path[SYSFS_PATH_MAX] = {0};
	char key[8] = {0};
	char *substr;
	int ret = -1;
	int result;

	if (in_path == NULL || key_str == NULL || out_path == NULL)
		return ret;
	strncpy_s(path, sizeof(path), in_path, strlen(in_path));

	if (strlen(key_str) == 0)
		return ret;
	strncpy_s(key, sizeof(key), key_str, sizeof(key)-1);

	while (1) {
		dir = opendir(path);
		if (NULL == dir) {
			break;
		}
		while (NULL != (dirent = readdir(dir))) {
			if (EOK == strcmp_s(dirent->d_name, strlen(dirent->d_name),
								".", &result)) {
				if (result == 0)
					continue;
			}
			if (EOK == strcmp_s(dirent->d_name, strlen(dirent->d_name),
								"..", &result)) {
				if (result == 0)
					continue;
			}

			if (EOK == strcmp_s(dirent->d_name, strlen(dirent->d_name),
								MACROM_DEVNAME, &result)) {
				if (result == 0) {
					snprintf_s_ss(out_path, size, "%s/%s", path,
								  dirent->d_name);
					ret = 0;
					break;
				}
			}
			if (EOK == strcmp_s(dirent->d_name, strlen(dirent->d_name),
								EEPROM_DEVNAME, &result)) {
				if (result == 0) {
					snprintf_s_ss(out_path, size, "%s/%s", path,
								  dirent->d_name);
					ret = 0;
					break;
				}
			}
			if (EOK == strstr_s(dirent->d_name, strlen(dirent->d_name),
								key, sizeof(key), &substr)) {
				snprintf_s_s(path+strlen(path), sizeof(path)-strlen(path),
							 "/%s", dirent->d_name);
				if (!strncmp(dirent->d_name, "i2c-", 4)) {
					sscanf_s_i(dirent->d_name, "i2c-%d", &result);
					snprintf_s_i(key, sizeof(key), "%d", result);
				}
				break;
			}
		}
		closedir(dir);
		if (dirent == NULL || ret == 0)
			break;
	}
	return ret;
}

static void print_mac_rom_info(fpga_properties props)
{
	char path[SYSFS_PATH_MAX];
	get_sysfs_path(props, FPGA_DEVICE, NULL);
	const char *sysfspath = get_sysfs_path(props, FPGA_DEVICE, NULL);
	int fd;
	int i, n;
	ssize_t sz;
	unsigned char buf[8];
	union {
		unsigned int dword;
		unsigned char byte[4];
	} mac;

	// Open FME device directory
	if (NULL == sysfspath) {
		fprintf(stderr, "WARNING: sysfs path not found\n");
		return;
	}
	if (0 != get_mac_rom_path(sysfspath, I2C_DEVNAME, path, SYSFS_PATH_MAX)) {
		fprintf(stderr, "WARNING: nvmem not found\n");
		return;
	}

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "Open %s failed\n", path);
		return;
	}
	sz = read(fd, buf, sizeof(buf));
	close(fd);

	if (sz == 0) {
		fprintf(stderr, "Read %s failed\n", path);
		return;
	}
	n = (int)buf[6];
	printf("%-29s : %d\n", "Number of MACs", n);
	mac.byte[0] = buf[5];
	mac.byte[1] = buf[4];
	mac.byte[2] = buf[3];
	mac.byte[3] = 0;
	for (i = 0; i < n; ++i) {
		printf("%s %-17d : %02X:%02X:%02X:%02X:%02X:%02X\n",
			   "MAC address", i, buf[0], buf[1], buf[2],
			   mac.byte[2], mac.byte[1], mac.byte[0]);
	    mac.dword += 1;
	}
}

static void print_mac_info(fpga_properties props)
{
	fpgainfo_print_common("//****** MAC ******//", props);
	print_mac_rom_info(props);
}

#define MAC_GETOPT_STRING ":h"
int parse_mac_args(int argc, char *argv[])
{
	struct option longopts[] = {
		{"help", no_argument, NULL, 'h'},
		{0, 0, 0, 0},
	};
	int getopt_ret;
	int option_index;

	optind = 0;
	while (-1 != (getopt_ret = getopt_long(argc, argv, MAC_GETOPT_STRING,
										   longopts, &option_index))) {
		const char *tmp_optarg = optarg;

		if (optarg && ('=' == *tmp_optarg)) {
			++tmp_optarg;
		}

		switch (getopt_ret) {
		case 'h':   /* help */
			mac_help();
			return -1;

		case ':':   /* missing option argument */
			fprintf(stderr, "Missing option argument\n");
			mac_help();
			return -1;

		case '?':
		default:    /* invalid option */
			fprintf(stderr, "Invalid cmdline options\n");
			mac_help();
			return -1;
		}
	}

	return 0;
}

fpga_result mac_filter(fpga_properties *filter, int argc, char *argv[])
{
	fpga_result res = FPGA_INVALID_PARAM;

	if (0 == parse_mac_args(argc, argv)) {
		res = fpgaPropertiesSetObjectType(*filter, FPGA_DEVICE);
		fpgainfo_print_err("setting type to FPGA_DEVICE", res);
	}
	return res;
}

fpga_result mac_command(fpga_token *tokens, int num_tokens, int argc,
						char *argv[])
{
	(void)argc;
	(void)argv;
	fpga_result res = FPGA_OK;
	fpga_properties props;

	int i = 0;
	for (i = 0; i < num_tokens; ++i) {
		res = fpgaGetProperties(tokens[i], &props);
		ON_FPGAINFO_ERR_GOTO(res, out_destroy, "reading properties from token");

		print_mac_info(props);
		fpgaDestroyProperties(&props);
	}

	return res;

out_destroy:
	fpgaDestroyProperties(&props);
	return res;
}

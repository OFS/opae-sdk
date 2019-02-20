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
#include "phyinfo.h"
#include "bmcinfo.h"
#include <opae/fpga.h>
#include <wchar.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include "safe_string/safe_string.h"

static int num_ports = 8;
static int speed = 10;

/*
 * phy command configuration, set during parse_phy_args()
 */
static struct _phy_config {
	int group_num;
	int show_pkvl;
} phy_config = {.group_num = -1,
				.show_pkvl = 1};

/*
 * Print help
 */
void phy_help(void)
{
	printf("\nPrint PHY information\n"
		   "        fpgainfo phy [-h] [-G <group-number>]\n"
		   "                -h,--help           Print this help\n"
		   "                -G,--group          Select PHY group {0,1,all}\n"
		   "\n");
}

#define PHYGRP_DEVNAME  "eth_group"
#define CDEV_ID_SIZE    8
#define FPGA_PHY_GROUP_GET_INFO 0xB702
static void print_phy_group_info(fpga_properties props, int group_num)
{
	DIR *dir = NULL;
	struct dirent *dirent = NULL;
	char path[SYSFS_PATH_MAX];
	const char *sysfspath = get_sysfs_path(props, FPGA_DEVICE, NULL);
	char *p;
	int i = 0;
	bool found = false;
	int fd;
	ssize_t sz;
	char cdevid[CDEV_ID_SIZE];

	printf("//****** PHY GROUP %d ******//\n", group_num);

	// Open misc directory under FME device
	snprintf_s_s(path, sizeof(path), "%s/misc/", sysfspath);
	dir = opendir(path);
	if (NULL == dir) {
		fprintf(stderr, "Can not open directory %s\n", path);
		return;
	}

	// Search specified PHY group
	while (NULL != (dirent = readdir(dir))) {
		if (!strcmp(dirent->d_name, "."))
			continue;
		if (!strcmp(dirent->d_name, ".."))
			continue;

		if (!strncmp(dirent->d_name, PHYGRP_DEVNAME, strlen(PHYGRP_DEVNAME))) {
			p = strrchr(dirent->d_name, '.');
			if (NULL == p) {
				fprintf(stderr, "Invalid eth_group name %s\n", dirent->d_name);
				continue;
			}
			++p;
			sscanf_s_i(p, "%d", &i);
			if (group_num == i) {
				strcat_s(path, sizeof(path), dirent->d_name);
				strcat_s(path, sizeof(path), "/dev");
				found = true;
				break;
			}
		}
	}

	closedir(dir);

	if (found) {
		// Get char device ID from PHY group sysfs device
		fd = open(path, O_RDONLY);
		if (fd < 0) {
			fprintf(stderr, "Open %s failed\n", path);
			return;
		}
		sz = read(fd, cdevid, CDEV_ID_SIZE);
		if (sz > 0)
			cdevid[sz-1] = '\0';
		close(fd);

		// Get information from PHY group char device
		snprintf_s_s(path, sizeof(path), "/dev/char/%s", cdevid);
		fd = open(path, O_RDWR);
		if (fd < 0) {
			fprintf(stderr, "Open %s failed\n", path);
			return;
		}
		struct fpga_phy_group_info {
			unsigned int    argsz;
			unsigned int    flags;
			unsigned char  speed;
			unsigned char  phy_num;
			unsigned char  mac_num;
			unsigned char  group_id;
		} info = {
			.argsz = 14,
			.flags = 0,
			.speed = 0,
			.phy_num = 0,
			.mac_num = 0,
			.group_id = 0
		};
		if (0 == ioctl(fd, FPGA_PHY_GROUP_GET_INFO, &info)) {
			printf("%-29s : %s\n", "Direction",
				   info.group_id == 0 ? "Line side" : "Fortville side");
			printf("%-29s : %d Gbps\n", "Speed", info.speed);
			printf("%-29s : %d\n", "Number of PHYs", info.phy_num);
		} else {
			printf("ioctl error\n");
		}
		if (info.group_id == 0) {
			num_ports = info.phy_num;
			speed = info.speed;
		}
		close(fd);
	} else {
		fprintf(stderr, "WARNING: eth_group %d not found\n", group_num);
	}
}

#define PKVL_NAME	"pkvl"
#define SPI_NAME	"spi"

static void print_pkvl_info(fpga_properties props)
{
	DIR *dir = NULL;
	struct dirent *dirent = NULL;
	char path[SYSFS_PATH_MAX] = {0};
	const char *sysfspath = get_sysfs_path(props, FPGA_DEVICE, NULL);
	char *substr;
	int found = 0;
	int offset;
	int shift;
	int result;
	int fd;
	int i;
	ssize_t ret;
	char mode[16] = {0};
	char status[16] = {0};

	printf("//****** PKVL ******//\n");

	if (sysfspath == NULL)
		return;
	strncpy_s(path, sizeof(path), sysfspath, strlen(sysfspath));

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
								PKVL_NAME, &result)) {
				if (result == 0) {
					snprintf_s_s(path+strlen(path), sizeof(path)-strlen(path),
								 "/%s", PKVL_NAME);
					found = 1;
					break;
				}
			}
			if (EOK == strstr_s(dirent->d_name, strlen(dirent->d_name),
								SPI_NAME, strlen(SPI_NAME), &substr)) {
				snprintf_s_s(path+strlen(path), sizeof(path)-strlen(path),
							 "/%s", dirent->d_name);
				break;
			}
		}
		closedir(dir);
		if (dirent == NULL || found == 1)
			break;
	}

	if (found) {
		offset = strlen(path);
		snprintf_s_s(path+offset, sizeof(path)-offset, "/%s", "polling_mode");
		get_sysfs_attr(path, mode, sizeof(mode));
		result = strtol(mode, NULL, 16);
		if (result == 0) {
			if (num_ports == 4) {
				result = 1;
			} else {
				result = 3;
			}
			/* set polling mode */
			fd = open(path, O_WRONLY);
			if (fd >= 0) {
				snprintf_s_i(mode, sizeof(mode), "%d", result);
				ret = write(fd, mode, strlen(mode));
				close(fd);
				if (ret > 0) {
					sleep(1);	// wait for polling
				} else {
					printf("write %s to %s failed\n", mode, path);
				}
			} else {
				printf("open %s failed\n", path);
			}
		}
		strncpy_s(mode, sizeof(mode), speed == 25 ? "25G" : "10G", 3);

		snprintf_s_s(path+offset, sizeof(path)-offset, "/%s", "status");
		get_sysfs_attr(path, status, sizeof(status));
		result = strtol(status, NULL, 16);
		shift = speed == 25 ? 4 : 1;
		for (i = 0; i < num_ports; i++) {
			printf("%s%-2d%-23s : %s\n", "Port",
				   i, mode, result&(1<<(i*shift)) ? "Up" : "Down");
		}
	} else {
		fprintf(stderr, "WARNING: pkvl not found\n");
	}
}

static void print_phy_info(fpga_properties props, struct _phy_config *config)
{
	fpgainfo_print_common("//****** PHY ******//", props);
	if ((config->group_num == 0) || (config->group_num == -1)) {
		print_phy_group_info(props, 0);
	}
	if ((config->group_num == 1) || (config->group_num == -1)) {
		print_phy_group_info(props, 1);
	}
	if (config->show_pkvl == 1) {
		print_pkvl_info(props);
	}
}

#define PHY_GETOPT_STRING ":G:h"
int parse_phy_args(int argc, char *argv[])
{
	struct option longopts[] = {
		{"group", required_argument, NULL, 'G'},
		{"help", no_argument, NULL, 'h'},
		{0, 0, 0, 0},
	};
	int getopt_ret;
	int option_index;

	/* default configuration */
	phy_config.group_num = -1;

	optind = 0;
	while (-1 != (getopt_ret = getopt_long(argc, argv, PHY_GETOPT_STRING,
										   longopts, &option_index))) {
		const char *tmp_optarg = optarg;

		if (optarg && ('=' == *tmp_optarg)) {
			++tmp_optarg;
		}

		switch (getopt_ret) {
		case 'G':
            if (NULL == tmp_optarg)
                break;
			if (!strcmp("0", tmp_optarg)) {
				phy_config.group_num = 0;
			} else if (!strcmp("1", tmp_optarg)) {
				phy_config.group_num = 1;
			} else if (!strcmp("all", tmp_optarg)) {
				phy_config.group_num = -1;
			} else {
				fprintf(stderr, "Invalid argument '%s' of option group\n",
						tmp_optarg);
				return -1;
			}
			break;

		case 'h':   /* help */
			phy_help();
			return -1;

		case ':':   /* missing option argument */
			fprintf(stderr, "Missing option argument\n");
			phy_help();
			return -1;

		case '?':
		default:    /* invalid option */
			fprintf(stderr, "Invalid cmdline options\n");
			phy_help();
			return -1;
		}
	}

	return 0;
}

fpga_result phy_filter(fpga_properties *filter, int argc, char *argv[])
{
	fpga_result res = FPGA_INVALID_PARAM;

	if (0 == parse_phy_args(argc, argv)) {
		res = fpgaPropertiesSetObjectType(*filter, FPGA_DEVICE);
		fpgainfo_print_err("setting type to FPGA_DEVICE", res);
	}
	return res;
}

fpga_result phy_command(fpga_token *tokens, int num_tokens, int argc,
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

		print_phy_info(props, &phy_config);
		fpgaDestroyProperties(&props);
	}

	return res;

out_destroy:
	fpgaDestroyProperties(&props);
	return res;
}

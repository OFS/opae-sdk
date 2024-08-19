// Copyright(c) 2024, Silciom Denmark A/S
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

#define _GNU_SOURCE 1
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <error.h>
#include <signal.h>
#include <getopt.h>
#include <opae/fpga.h>
#include "argsfilter.h"

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef QSFPPRINT
// From ethtool
#undef HAVE_CONFIG_H
#include "qsfp.c"

// Read QSFP SFF-8636 registers into memory and setup pointers so
// sff8636_show_all_common() from ethertool can print result
static int sff8636_show(const char * regmap_path)
{

	struct sff8636_memory_map map = {};
	__u8 buffer[1024];
	int i = 0; // counting lines read
	int bufp = 0; // counting bytes written to buffer
	uint32_t value;

	char *line = NULL;
	size_t len = 0;
	ssize_t read;
	char delim[] = ":";

	FILE *fp = fopen (regmap_path, "r");
	if (!fp) {
		OPAE_ERR("Could not open regmap: %s ", regmap_path);
		return ENOENT;
	}

	while ((read = getline(&line, &len, fp)) != -1)
	{
		//The first 4 lines are not used
		if (i >= 4)
		{
			// Line has the form <addr> : <32 bit little endian value>
			char *ptr = strtok(line, delim);
			ptr = strtok(NULL, delim);
			value = strtol(ptr, NULL, 16);
			//printf("value: 0x%x\n", value);
			uint32_t *buftemp = (uint32_t *) &buffer[bufp];
			*buftemp = value;
			bufp += 4;
		}
		i++;
	}

	map.lower_memory = buffer; // address 0x100 in regmap
	map.page_00h = buffer + (0x200 - 0x80); //starts from beginning of lower page 128 bytes before
	map.page_03h = buffer + 0x200; //starts 128 bytes before

	// Print info
	sff8636_show_all_common(&map);
	fclose (fp);
	return 0;
}
#endif

// Define the function to be called when ctrl-c (SIGINT) signal is sent to
// process
static volatile bool stop = false;
void signal_callback_handler(int signum)
{
	switch (signum) {
	case SIGINT:
		stop = true;
		break;
	default:
		break;
	}
}

#define MAX_PORT  2
#define NO_SPECIFIC_PORT (MAX_PORT+1)
#define MAX_CARDS 4

struct card {
	fpga_properties filter;
	fpga_token token[MAX_CARDS];
	int32_t port;
	uint32_t cards_found;
};

static fpga_result fpga_open(struct card *card)
{
	fpga_result res;
	res = fpgaPropertiesSetObjectType(card->filter, FPGA_DEVICE);
	if (res != FPGA_OK) {
		OPAE_ERR("failed to set object type: %s ",	fpgaErrStr(res));
		goto error;
	}

	res = fpgaEnumerate(&card->filter, 1, card->token, MAX_CARDS, &card->cards_found);
	if (res != FPGA_OK) {
		OPAE_ERR("failed to enumerate fpga: %s ", fpgaErrStr(res));
		goto error;
	}

	if (card->cards_found == 0) {
		res = FPGA_NOT_FOUND;
		OPAE_ERR("failed to find fpga: %s ", fpgaErrStr(res));
		goto error;
	}

error:
	return res;
}

static void fpga_close(struct card *card)
{
	fpga_result res;

	for(uint32_t i = 0; i < card->cards_found; i++) {
		res = fpgaDestroyToken(&card->token[i]);
		if (res != FPGA_OK)
			OPAE_ERR("failed to destroy token: %s ", fpgaErrStr(res));
	}
}

// QSFP cable status
#define LPATH_MAX 100
#define DFL_SYSFS_QSFP "*dfl*dev.%ld/qsfp_connected"
#define REGMAP_PATH "/sys/kernel/debug/regmap/dfl_dev.%ld/registers"
#define MAX_DEV_FEATURE_COUNT 256

// from opae board_common.c extended  with sff8636_show()
fpga_result qsfp_cable_status(const fpga_token token, const size_t port)
{
	fpga_object fpga_object;
	fpga_result res              = FPGA_OK;
	uint64_t value               = 0;
	size_t i                     = 0;
	char qsfp_path[LPATH_MAX]     = { 0 };
	int retval                   = 0;
	size_t qsfp_count            = 0;

	for (i = 0; i < MAX_DEV_FEATURE_COUNT; i++) {

		memset(qsfp_path, 0, sizeof(qsfp_path));

		retval = snprintf(qsfp_path, sizeof(qsfp_path),
			DFL_SYSFS_QSFP, i);
		if (retval < 0) {
			OPAE_MSG("error in formatting qsfp cable status");
			return FPGA_EXCEPTION;
		}

		res = fpgaTokenGetObject(token, qsfp_path,
			&fpga_object, FPGA_OBJECT_GLOB);
		if (res != FPGA_OK) {
			OPAE_MSG("Failed to get token Object");
			continue;
		}

		res = fpgaObjectRead64(fpga_object, &value, 0);
		if (res == FPGA_OK) {
			if (port == NO_SPECIFIC_PORT || port == qsfp_count) {
				OPAE_MSG("Failed to Read object ");

				switch (value) {
				case 0:
					printf("QSFP%-45ld : %s \n", qsfp_count, "Not Connected");
					break;
				case 1:
					printf("QSFP%-45ld : %s \n", qsfp_count, "Connected");

#ifdef QSFPPRINT
					char regmap_path[LPATH_MAX]   = { 0 };
					retval = snprintf(regmap_path, sizeof(regmap_path),
						REGMAP_PATH, i);
					if (retval < 0) {
						OPAE_MSG("error in formatting qsfp regmap");
						return FPGA_EXCEPTION;
					}
					sff8636_show(regmap_path);
#endif
					break;
				default:
					printf("QSFP%-28ld : %s \n", qsfp_count, "N/A");
				}
			}
			qsfp_count++;

		} else {
			OPAE_MSG("Failed to Read object ");
		}

		res = fpgaDestroyObject(&fpga_object);
		if (res != FPGA_OK) {
			OPAE_MSG("Failed to Destroy Object");
		}

	}

	return res;
}

// from opae board_common.c
fpga_result get_fpga_sbdf(fpga_token token,
			uint16_t *segment,
			uint8_t *bus,
			uint8_t *device,
			uint8_t *function)

{
	fpga_result res = FPGA_OK;
	fpga_properties props = NULL;
	fpga_result resval = FPGA_OK;

	if (!segment || !bus ||
		!device || !function) {
		OPAE_ERR("Invalid input parameters");
		return FPGA_INVALID_PARAM;
	}
	res = fpgaGetProperties(token, &props);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get properties ");
		return res;
	}

	res = fpgaPropertiesGetBus(props, bus);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get bus ");
		resval = res;
		goto out_destroy;
	}

	res = fpgaPropertiesGetSegment(props, segment);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get Segment ");
		resval = res;
		goto out_destroy;
	}
	res = fpgaPropertiesGetDevice(props, device);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get Device ");
		resval = res;
		goto out_destroy;
	}

	res = fpgaPropertiesGetFunction(props, function);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get Function ");
		resval = res;
		goto out_destroy;
	}

out_destroy:
	res = fpgaDestroyProperties(&props);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to destroy properties");
	}

	return resval;
}


static void print_usage(FILE *f)
{
	fprintf(f,
		   "\n"
		   "qsfpinfo\n"
		   "Print QSFP EEPROM information\n"
		   "\n");
	fprintf(f, "Usage:\n");
	fprintf(f, "        qsfpinfo [-h] [-p <port>] [PCI_ADDR]\n");
	fprintf(f,"\n");
	fprintf(f,
		   "                -p,--port           Print only information for this QSFP [0..1]\n"
		   "                -S,--segment        Set target segment number\n"
		   "                -B,--bus            Set target bus number\n"
		   "                -D,--device         Set target device number\n"
		   "                -F,--function       Set target function number\n"
		   "\n");
}


static bool parse_port(struct card *card, const char *port_str)
{
	char *endptr;
	unsigned long int port = strtoul(port_str, &endptr, 0);
	if (port_str == endptr) {
		OPAE_ERR("missing port number: '%s'", port_str);
		return false;
	}

	if (port >= MAX_PORT) {
		OPAE_ERR("port number too big: '%d'", port);
		return false;
	}

	card->port = port;
	return true;
}


static int parse_args(int argc, char **argv, struct card *card)
{
	struct option options[] = {
		{"help", no_argument, NULL, 'h'},
		{"port", required_argument, NULL, 'p'},
		{NULL, 0, NULL, 0},
	};

	int c;

	while (1) {
		c = getopt_long_only(argc, argv, "hg:m:p:d", options, NULL);
		if (c == -1)
			break;

		switch (c) {
		case 'h':
			print_usage(stdout);
			exit(EXIT_SUCCESS);
		case 'p':
			if (!parse_port(card, optarg))
				return EXIT_FAILURE;
			break;
		case '?':
		default:
			return EXIT_FAILURE;
		}
	}

	return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
	struct card card = {.filter = NULL, .port = NO_SPECIFIC_PORT, .cards_found = 0};
	uint16_t segment;
	uint8_t bus;
	uint8_t device;
	uint8_t function;
	fpga_result res;
	int err;

	res = fpgaGetProperties(NULL, &card.filter);
	if (res != FPGA_OK) {
		OPAE_ERR("failed to get properties: %s ", fpgaErrStr(res));
		return res;
	}

	if (opae_set_properties_from_args(card.filter, &res, &argc, argv)) {
		OPAE_ERR("failed arg parse.");
		res = FPGA_EXCEPTION;
		goto error2;
	} else if (res) {
		OPAE_ERR("failed to set properties.");
		goto error2;
	}

	err = parse_args(argc, argv, &card);
	if (err != 0)
		goto error2;

	// Install Control-C handler
	signal(SIGINT, signal_callback_handler);

	res = fpga_open(&card);
	if (res != FPGA_OK)
		goto error;

	for(uint32_t i = 0; i < card.cards_found; i++) {
		get_fpga_sbdf(card.token[i], &segment, &bus, &device, &function);
		printf("%-49s : %04X:%02X:%02X.%01X\n", "PCIe s:b:d.f", segment, bus, device, function);
		res = qsfp_cable_status(card.token[i], card.port);
	}

error:
	fpga_close(&card);

error2:
	fpgaDestroyProperties(&card.filter);

	return res == FPGA_OK ? EXIT_SUCCESS : res;
}

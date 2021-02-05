// Copyright(c) 2021, Silciom Denmark A/S
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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <error.h>
#include <time.h>
#include <getopt.h>
#include <opae/fpga.h>
#include <uuid/uuid.h>
#include <endian.h>

#define DFH_EOL(h)    ((h >> 40) & 1)
#define DFH_NEXT(h)   ((h >> 16) & 0xffffff)
#define DFH_TYPE(h)   ((h >> 60) & 0xf)
#define DFH_TYPE_AFU  1

#define DDR_TEST_MODE0_CTRL          0x3000
#define DDR_TEST_MODE0_STAT          0x3008
#define DDR_TEST_MODE0_FERR_ADDR(x) (0x3010 + 8 * x)

#define DDR_TEST_MODE1_CTRL          0x3000
#define DDR_TEST_MODE1_STAT          0x3008
#define DDR_TEST_MODE1_BANK_STAT(x) (0x3010 + 8 * x)

#define DDR_TEST_HBM_PASS     0x4000
#define DDR_TEST_HBM_FAIL     0x4008
#define DDR_TEST_HBM_TIMEOUT  0x4010

struct n5010;

struct n5010_test {
	const char *name;
	fpga_result (*func)(struct n5010 *n5010);
};

struct n5010 {
	fpga_token token;
	fpga_handle handle;
	fpga_guid guid;
	uint64_t base;
	const struct n5010_test *test;
	bool debug;
};

static fpga_result fpga_test_directed(struct n5010 *n5010);
static fpga_result fpga_test_prbs(struct n5010 *n5010);
static fpga_result fpga_test_hbm(struct n5010 *n5010);

static const struct n5010_test n5010_test[] = {
	{
		.name = "directed",
		.func = fpga_test_directed,
	},
	{
		.name = "prbs",
		.func = fpga_test_prbs,
	},
	{
		.name = "hbm",
		.func = fpga_test_hbm,
	},
};

static fpga_result fpga_open(struct n5010 *n5010)
{
	fpga_properties filter = NULL;
	fpga_result res;
	uint32_t num = 0;

	res = fpgaGetProperties(NULL, &filter);
	if (res != FPGA_OK) {
		fprintf(stderr, "failed to get properties: %s\n", fpgaErrStr(res));
		goto error;
	}

	res = fpgaPropertiesSetObjectType(filter, FPGA_ACCELERATOR);
	if (res != FPGA_OK) {
		fprintf(stderr, "failed to set object type: %s\n", fpgaErrStr(res));
		goto error;
	}

	res = fpgaPropertiesSetGUID(filter, n5010->guid);
	if (res != FPGA_OK) {
		fprintf(stderr, "failed to set guid: %s\n", fpgaErrStr(res));
		goto error;
	}

	res = fpgaEnumerate(&filter, 1, &n5010->token, 1, &num);
	if (res != FPGA_OK) {
		fprintf(stderr, "failed to enumerate fpga: %s\n", fpgaErrStr(res));
		goto error;
	}

	res = fpgaOpen(n5010->token, &n5010->handle, 0);
	if (res != FPGA_OK) {
		fprintf(stderr, "failed to open fpga: %s\n", fpgaErrStr(res));
		goto error;
	}

	res = fpgaMapMMIO(n5010->handle, 0, NULL);
	if (res != FPGA_OK) {
		fprintf(stderr, "failed to map io memory: %s\n", fpgaErrStr(res));
		goto error;
	}

	res = fpgaReset(n5010->handle);
	if (res != FPGA_OK) {
		fprintf(stderr, "failed to reset fpga: %s\n", fpgaErrStr(res));
		goto error;
	}

error:
	fpgaDestroyProperties(&filter);

	return res;
}

static void fpga_close(struct n5010 *n5010)
{
	fpga_result res;

	if (n5010->handle != NULL) {
		res = fpgaUnmapMMIO(n5010->handle, 0);
		if (res != FPGA_OK)
			fprintf(stderr, "failed to unmap io memory: %s\n", fpgaErrStr(res));
	}

	if (n5010->handle != NULL) {
		res = fpgaClose(n5010->handle);
		if (res != FPGA_OK)
			fprintf(stderr, "failed to close fpga: %s\n", fpgaErrStr(res));
	}

	if (n5010->token != NULL) {
		res = fpgaDestroyToken(&n5010->token);
		if (res != FPGA_OK)
			fprintf(stderr, "failed to destroy token: %s\n", fpgaErrStr(res));
	}
}

static fpga_result fpga_base(struct n5010 *n5010)
{
	fpga_result res;

	while (1) {
		uint64_t header;
		struct {
			union {
				uuid_t u128;
				uint64_t u64[2];
			};
		} uuid;
		int i;

		res = fpgaReadMMIO64(n5010->handle, 0, n5010->base, &header);
		if (res != FPGA_OK) {
			fprintf(stderr, "failed to read dfh: %s\n", fpgaErrStr(res));
			break;
		}

		res = fpgaReadMMIO64(n5010->handle, 0, n5010->base + 8, &uuid.u64[1]);
		if (res != FPGA_OK) {
			fprintf(stderr, "failed to read uuid high: %s\n", fpgaErrStr(res));
			break;
		}

		res = fpgaReadMMIO64(n5010->handle, 0, n5010->base + 16, &uuid.u64[0]);
		if (res != FPGA_OK) {
			fprintf(stderr, "failed to read uuid low: %s\n", fpgaErrStr(res));
			break;
		}

		for (i = 0; i < 2; i++)
			uuid.u64[i] = be64toh(uuid.u64[i]);

		if (DFH_TYPE(header) == DFH_TYPE_AFU &&
		    uuid_compare(uuid.u128, n5010->guid) == 0)
			break;

		if (DFH_EOL(header)) {
			fprintf(stderr, "no matching dfh found: 0x%016jx\n", header);
			res = FPGA_NOT_FOUND;
			break;
		}

		if (DFH_NEXT(header) == 0 || DFH_NEXT(header) == 0xffff) {
			fprintf(stderr, "next dfh not found: 0x%016jx\n", header);
			res = FPGA_NOT_FOUND;
			break;
		}

		n5010->base += DFH_NEXT(header);
	}

	return res;
}

static void fpga_dump(struct n5010 *n5010, uint64_t offset, size_t count)
{
	uint64_t base = n5010->base + offset;

	if (!n5010->debug)
		return;

	for (size_t i = 0; i < count; i++) {
		uint64_t reg = base + i * 8;
		uint64_t val;

		fpgaReadMMIO64(n5010->handle, 0, reg, &val);
		printf("reg: 0x%04jx, val: 0x%016jx\n", reg, val);
	}
}

static fpga_result fpga_banks(struct n5010 *n5010, uint64_t offset, uint64_t *banks)
{
	fpga_result res = FPGA_OK;

	res = fpgaReadMMIO64(n5010->handle, 0, n5010->base + offset, banks);
	if (res != FPGA_OK) {
		fprintf(stderr, "failed to read banks: %s\n", fpgaErrStr(res));
		goto error;
	}

error:
	return res;
}

static fpga_result fpga_start(struct n5010 *n5010, uint64_t offset, uint64_t banks)
{
	uint64_t ctrl;
	fpga_result res = FPGA_OK;

	ctrl = (1 << banks) - 1;
	printf("starting\n");

	res = fpgaWriteMMIO64(n5010->handle, 0, n5010->base + offset, ctrl);
	if (res != FPGA_OK) {
		fprintf(stderr, "failed to start test: %s\n", fpgaErrStr(res));
		goto error;
	}

	res = fpgaReadMMIO64(n5010->handle, 0, n5010->base + offset, &ctrl);
	if (res != FPGA_OK) {
		fprintf(stderr, "failed to read banks: %s\n", fpgaErrStr(res));
		goto error;
	}

	fpga_dump(n5010, offset, 1);

error:
	return res;
}
static fpga_result fpga_wait(struct n5010 *n5010, uint64_t offset, uint64_t init, uint64_t result)
{
	struct timespec ts = { .tv_sec = 0, .tv_nsec = 100000000 };
	uint64_t status = init;
	fpga_result res;

	printf("waiting\n");

	while (status == init) {
		nanosleep(&ts, NULL);

		res = fpgaReadMMIO64(n5010->handle, 0, n5010->base + offset, &status);
		if (res != FPGA_OK) {
			fprintf(stderr, "failed to read status: %s\n", fpgaErrStr(res));
			goto error;
		}

		if (n5010->debug)
			printf("reg: 0x%04jx, val: 0x%016jx\n", offset, status);
	}

	if (status != result) {
		fprintf(stderr, "failed with unexpected result: expected: 0x%016jx, got: 0x%016jx\n",
			result, status);
		res = FPGA_EXCEPTION;
		goto error;
	}

error:
	return res;
}

static fpga_result fpga_test_directed(struct n5010 *n5010)
{
	uint64_t banks;
	fpga_result res;

	res = fpga_banks(n5010, DDR_TEST_MODE0_STAT, &banks);
	if (res != FPGA_OK)
		goto error;

	res = fpga_start(n5010, DDR_TEST_MODE0_CTRL, banks);
	if (res != FPGA_OK)
		goto error;

	res = fpga_wait(n5010, DDR_TEST_MODE0_STAT, banks, 0x10101);
	if (res != FPGA_OK)
		goto error;

	fpga_dump(n5010, DDR_TEST_MODE0_FERR_ADDR(0), banks);

error:
	return res;
}

static fpga_result fpga_test_prbs(struct n5010 *n5010)
{
	uint64_t banks;
	fpga_result res;

	res = fpga_banks(n5010, DDR_TEST_MODE1_STAT, &banks);
	if (res != FPGA_OK)
		goto error;

	res = fpga_start(n5010, DDR_TEST_MODE1_CTRL, banks);
	if (res != FPGA_OK)
		goto error;

	while (banks--) {
		res = fpga_wait(n5010, DDR_TEST_MODE1_BANK_STAT(banks), 0LLU, 0x1);
		if (res != FPGA_OK)
			goto error;
	}

error:
	return res;
}

static fpga_result fpga_test_hbm(struct n5010 *n5010)
{
	fpga_result res;

	res = fpga_wait(n5010, DDR_TEST_HBM_TIMEOUT, 0, 0);

	return res;
}

static void print_usage(FILE *f)
{
	fprintf(f,
		"usage: %s [<options>]\n"
		"\n"
		"options:\n"
		"  --help   print this help\n"
		"  --guid   uuid of accelerator to open\n"
		"  --mode   test mode to execute. Known modes:\n"
		"             directed, prbs, hbm\n"
		"  --debug  enable debug print of register values\n"
		"\n", program_invocation_short_name);
}

static bool parse_mode(struct n5010 *n5010, const char *mode)
{
	const struct n5010_test *t;
	size_t count = sizeof(n5010_test)/sizeof(*n5010_test);
	size_t i;

	for (i = 0; i < count; i++) {
		t = &n5010_test[i];

		if (strcmp(t->name, mode) == 0)
			break;
	}

	if (i == count) {
		fprintf(stderr, "invalid mode setting: '%s'\n", mode);
		return false;
	}

	n5010->test = t;

	return true;
}

static int parse_args(int argc, char **argv, struct n5010 *n5010)
{
	struct option options[] = {
		{ "help",  no_argument,       NULL, 'h' },
		{ "guid",  required_argument, NULL, 'g' },
		{ "mode",  required_argument, NULL, 'm' },
		{ "debug", no_argument,       NULL, 'd' },
		{ NULL,    0,                 NULL, 0   },
	};

	int c;

	while (1) {
		c = getopt_long_only(argc, argv, "hg:m:d", options, NULL);
		if (c == -1)
			break;

		switch (c) {
			case 'h':
				print_usage(stdout);
				exit(EXIT_SUCCESS);
			case 'g':
				if (uuid_parse(optarg, n5010->guid) < 0) {
					fprintf(stderr, "unparsable uuid: '%s'\n", optarg);
					return EXIT_FAILURE;
				}
				break;
			case 'm':
				if (!parse_mode(n5010, optarg))
					return EXIT_FAILURE;
				break;
			case 'd':
				n5010->debug = true;
				break;
			case '?':
			default:
				return EXIT_FAILURE;
		}
	}

	if (uuid_is_null(n5010->guid)) {
		fprintf(stderr, "no guid given\n");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
	struct n5010 n5010 = { .test = n5010_test };
	fpga_result res;
	int err;

	err = parse_args(argc, argv, &n5010);
	if (err != 0)
		return err;

	res = fpga_open(&n5010);
	if (res != FPGA_OK)
		goto error;

	res = fpga_base(&n5010);
	if (res != FPGA_OK)
		goto error;

	fpga_dump(&n5010, 0, 3);
	fpga_dump(&n5010, 0x3000, 6);

	res = n5010.test->func(&n5010);
	if (res != FPGA_OK)
		goto error;

	printf("passed\n");

error:
	fpga_close(&n5010);

	return res == FPGA_OK ? EXIT_SUCCESS : res;
}

// Copyright(c) 2021-2022 , Silicom Denmark A/S
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

// compile: gcc -DSTATIC=static -D_GNU_SOURCE -I /usr/src/opae/argsfilter /usr/src/opae/argsfilter/argsfilter.c n5010-test.c -o n5010-test -l opae-c -l uuid

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <error.h>
#include <time.h>
#include <getopt.h>
#include <opae/fpga.h>
#include <uuid/uuid.h>
#include <endian.h>

#include <argsfilter.h>

#define DFH_EOL(h)    ((h >> 40) & 1)
#define DFH_NEXT(h)   ((h >> 16) & 0xffffff)
#define DFH_TYPE(h)   ((h >> 60) & 0xf)
#define DFH_TYPE_AFU  1

#define DDR_TEST_MODE0_CTRL          0x3000
#define DDR_TEST_MODE0_STAT          0x3008

#define DDR_TEST_MODE1_CTRL          0x3000
#define DDR_TEST_MODE1_STAT          0x3008
#define DDR_TEST_MODE1_BANK_STAT(x) (0x3010 + 8 * x)

#define HBM_TEST_PASS     0x4000
#define HBM_TEST_FAIL     0x4008
#define HBM_TEST_TIMEOUT  0x4010
#define HBM_TEST_CTRL     0x4018

#define QDR_TEST_STAT       0x5000
#define QDR_FM              0xff
#define QDR_TEST_TIMEOUT(h) ((h >> 16) & QDR_FM)
#define QDR_TEST_FAIL(h)    ((h >>  8) & QDR_FM)
#define QDR_TEST_PASS(h)    ((h >>  0) & QDR_FM)

#define QDR_TEST_CTRL       0x5008

#define ESRAM_TEST_STAT       0x6000
#define ESRAM_FM              0xffff
#define ESRAM_TEST_TIMEOUT(h) ((h >> 32) & ESRAM_FM)
#define ESRAM_TEST_FAIL(h)    ((h >> 16) & ESRAM_FM)
#define ESRAM_TEST_PASS(h)    ((h >>  0) & ESRAM_FM)

#define ESRAM_TEST_CTRL    0x6008

#define MAX_POLLS 10

struct n5010;

struct n5010_test {
	const char *name;
	fpga_result (*func)(struct n5010 *n5010);
};

struct n5010 {
	fpga_token token;
	fpga_handle handle;
	fpga_guid guid;
	fpga_properties filter;
	uint64_t base;
	const struct n5010_test *test;
	bool debug;
	uint open_mode;
};

static fpga_result fpga_test_ddr_directed(struct n5010 *n5010);
static fpga_result fpga_test_ddr_prbs(struct n5010 *n5010);
static fpga_result fpga_test_hbm(struct n5010 *n5010);
static fpga_result fpga_test_esram(struct n5010 *n5010);
static fpga_result fpga_test_qdr(struct n5010 *n5010);

static const struct n5010_test n5010_test[] = {
	{
		.name = "hbm",
		.func = fpga_test_hbm,
	},
	{
		.name = "ddr-directed",
		.func = fpga_test_ddr_directed,
	},
	{
		.name = "ddr-prbs",
		.func = fpga_test_ddr_prbs,
	},
	{
		.name = "esram",
		.func = fpga_test_esram,
	},
	{
		.name = "qdr",
		.func = fpga_test_qdr,
	}
};

static fpga_result fpga_open(struct n5010 *n5010)
{
	fpga_result res;
	uint32_t num = 0;

	res = fpgaPropertiesSetObjectType(n5010->filter, FPGA_ACCELERATOR);
	if (res != FPGA_OK) {
		fprintf(stderr, "failed to set object type: %s\n", fpgaErrStr(res));
		return res;
	}

	res = fpgaPropertiesSetGUID(n5010->filter, n5010->guid);
	if (res != FPGA_OK) {
		fprintf(stderr, "failed to set guid: %s\n", fpgaErrStr(res));
		return res;
	}

	res = fpgaEnumerate(&n5010->filter, 1, &n5010->token, 1, &num);
	if (res != FPGA_OK) {
		fprintf(stderr, "failed to enumerate fpga: %s\n", fpgaErrStr(res));
		return res;
	}

	if (num == 0) {
		res = FPGA_NOT_FOUND;
		fprintf(stderr, "failed to find fpga with guid: %s\n", fpgaErrStr(res));
		return res;
	}

	res = fpgaOpen(n5010->token, &n5010->handle, n5010->open_mode);
	if (res != FPGA_OK) {
		fprintf(stderr, "failed to open fpga: %s\n", fpgaErrStr(res));
		return res;
	}

	res = fpgaMapMMIO(n5010->handle, 0, NULL);
	if (res != FPGA_OK) {
		fprintf(stderr, "failed to map io memory: %s\n", fpgaErrStr(res));
		return res;
	}

	res = fpgaReset(n5010->handle);
	if (res != FPGA_OK) {
		fprintf(stderr, "failed to reset fpga: %s\n", fpgaErrStr(res));
		return res;
	}

	return res;
}

static void fpga_close(struct n5010 *n5010)
{
	fpga_result res;

	res = fpgaDestroyProperties(&n5010->filter);
	if (res != FPGA_OK)
		fprintf(stderr, "failed to destroy device filter: %s\n", fpgaErrStr(res));

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

		if (DFH_TYPE(header) == DFH_TYPE_AFU && uuid_compare(uuid.u128, n5010->guid) == 0)
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

static fpga_result fpga_banks(struct n5010 *n5010, uint64_t offset, uint64_t *num_banks)
{
	fpga_result res = FPGA_OK;

	res = fpgaReadMMIO64(n5010->handle, 0, n5010->base + offset, num_banks);
	if (res != FPGA_OK) {
		fprintf(stderr, "failed to read num_banks: %s\n", fpgaErrStr(res));
		goto error;
	}

error:
	return res;
}

static fpga_result fpga_start(struct n5010 *n5010, uint64_t offset, uint64_t num_banks)
{
	uint64_t ctrl;
	fpga_result res = FPGA_OK;

	ctrl = ((uint64_t)1 << num_banks) - 1;
	printf("starting PRBS generators\n");

	fpga_dump(n5010, offset, 1);

	res = fpgaWriteMMIO64(n5010->handle, 0, n5010->base + offset, ctrl);
	if (res != FPGA_OK) {
		fprintf(stderr, "failed to start PRBS generators: %s\n", fpgaErrStr(res));
		goto error;
	}

	usleep(1000000);

	fpga_dump(n5010, offset, 1);

error:
	return res;
}

static fpga_result fpga_stop(struct n5010 *n5010, uint64_t offset)
{
	uint64_t ctrl = 0;
	fpga_result res = FPGA_OK;

	printf("stopping DDR test generators\n");
	fpga_dump(n5010, offset, 1);

	res = fpgaWriteMMIO64(n5010->handle, 0, n5010->base + offset, ctrl);
	if (res != FPGA_OK) {
		fprintf(stderr, "failed to stop DDR test generators: %s\n", fpgaErrStr(res));
		goto error;
	}

	usleep(1000000);

	fpga_dump(n5010, offset, 1);

error:
	return res;
}

static fpga_result fpga_wait(struct n5010 *n5010, uint64_t offset, uint64_t init, uint64_t result)
{
	struct timespec ts = {.tv_sec = 0, .tv_nsec = 100000000};
	uint64_t status = init;
	fpga_result res;

	printf("waiting for test to complete...\n");

	while (status == init) {
		nanosleep(&ts, NULL);

		res = fpgaReadMMIO64(n5010->handle, 0, n5010->base + offset, &status);
		if (res != FPGA_OK) {
			fprintf(stderr, "failed to read status: %s\n", fpgaErrStr(res));
			goto error;
		}

		if (n5010->debug)
			printf("reg: 0x%04jx (DDR status), val: 0x%016jx\n", offset, status);
	}

	if (status != result) {
		fprintf(stderr, "failed with unexpected result: expected: 0x%016jx, got: 0x%016jx\n", result, status);
		res = FPGA_EXCEPTION;
		goto error;
	}

error:
	return res;
}

static fpga_result fpga_test_ddr_prbs(struct n5010 *n5010)
{
	uint64_t num_banks, stat, i;
	fpga_result res;

	printf("starting DDR PRBS read/write test\n");

	fpga_dump(n5010, DDR_TEST_MODE1_CTRL, 6);

	// Read number of num_banks from stat
	res = fpga_banks(n5010, DDR_TEST_MODE1_STAT, &num_banks);
	if (res != FPGA_OK)
		goto error;

	// Clear PRBS start bits
	res = fpga_stop(n5010, DDR_TEST_MODE1_CTRL);
	if (res != FPGA_OK)
		goto error;

	// Expect DDR_TEST_MODE1_BANK_STAT(bank) to return 0 while PRBS not running
	for (i = 0; i < num_banks; i++) {
		res = fpgaReadMMIO64(n5010->handle, 0, n5010->base + DDR_TEST_MODE1_BANK_STAT(i), &stat);
		if (res != FPGA_OK) {
			fprintf(stderr, "failed to read DDR_TEST_MODE1_BANK_STAT(%ju): %s\n", i, fpgaErrStr(res));
			goto error;
		}

		if (stat != 0x0) {
			fprintf(stderr, "Error: PRBS stat non-zero while generator idle: 0x%016jx\n", stat);
			res = FPGA_EXCEPTION;
			goto error;
		}
	}

	// Set PRBS start bits for each bank
	res = fpga_start(n5010, DDR_TEST_MODE1_CTRL, num_banks);
	if (res != FPGA_OK)
		goto error;

	// Wait for PRBS pass
	for (i = 0; i < num_banks; i++) {
		res = fpga_wait(n5010, DDR_TEST_MODE1_BANK_STAT(i), 0, 0x39);
		if (res != FPGA_OK)
			goto error;
	}

error:
	return res;
}

static fpga_result fpga_test_ddr_directed(struct n5010 *n5010)
{
	uint64_t num_banks, stat;
	fpga_result res;

	printf("starting DDR directed read/write test\n");

	fpga_dump(n5010, DDR_TEST_MODE0_CTRL, 4);

	// Read number of num_banks from stat
	res = fpga_banks(n5010, DDR_TEST_MODE0_STAT, &num_banks);
	if (res != FPGA_OK)
		goto error;

	// Clear test generator start bits
	res = fpga_stop(n5010, DDR_TEST_MODE0_CTRL);
	if (res != FPGA_OK)
		goto error;

	// Expect error status to return 0 while test generator is not running
	res = fpgaReadMMIO64(n5010->handle, 0, n5010->base + DDR_TEST_MODE0_STAT, &stat);
	if (res != FPGA_OK) {
		fprintf(stderr, "Error: failed to read DDR_TEST_MODE0_STAT: %s\n", fpgaErrStr(res));
		goto error;
	}
	// Check test error bits [23:16]
	if ((stat & 0xFF0000) != 0x0) {
		fprintf(stderr, "Error: test generator error status non-zero while generator idle: 0x%016jx\n", stat);
		res = FPGA_EXCEPTION;
		goto error;
	}

	// Set start bits for each bank
	res = fpga_start(n5010, DDR_TEST_MODE0_CTRL, num_banks);
	if (res != FPGA_OK)
		goto error;

	// Wait for test generator pass bits to indicate done for all banks:
	// DDR_TEST_MODE0_STAT[23:16] : test error = 0x0
	// DDR_TEST_MODE0_STAT[15:8]  : test done  = (1 << num_banks)-1
	// DDR_TEST_MODE0_STAT[7:0]   : # banks    = num_banks
	res = fpga_wait(n5010, DDR_TEST_MODE0_STAT, num_banks, (((1 << num_banks)-1) << 8) + num_banks);
	if (res != FPGA_OK)
		goto error;

	// Check for reported errors (DDR_TEST_MODE0_STAT[23:16])
	res = fpgaReadMMIO64(n5010->handle, 0, n5010->base + DDR_TEST_MODE0_STAT, &stat);
	if (res != FPGA_OK) {
		fprintf(stderr, "Error: failed to read DDR_TEST_MODE0_STAT: %s\n", fpgaErrStr(res));
		goto error;
	}
	if ((stat & 0xFF0000) != 0x0) {
		fprintf(stderr, "Error: Test failed with the following status: 0x%016jx\n", stat);
		res = FPGA_EXCEPTION;
		goto error;
	}

error:
	return res;
}

static fpga_result fpga_test_esram(struct n5010 *n5010)
{
	struct timespec ts = {.tv_sec = 0, .tv_nsec = 100000000};
	uint64_t stat = 0;
	uint16_t pass = 0;
	uint16_t fail = 0;
	uint16_t timeout = 0;
	fpga_result res ;
	uint32_t polls = 0;

	printf("starting eSRAM read/write test\n");

	res = fpgaReadMMIO64(n5010->handle, 0, n5010->base + ESRAM_TEST_STAT, &stat);
	if (res != FPGA_OK || stat != 0) {
		fprintf(stderr, "FPGA not ready for test, status: 0x%016jx\n", stat);
		if (res == FPGA_OK)
			res = FPGA_EXCEPTION;
		goto error;
	}

	res = fpgaWriteMMIO64(n5010->handle, 0, n5010->base + ESRAM_TEST_CTRL, 1);
	if (res != FPGA_OK) {
		fprintf(stderr, "failed to start test: %s\n", fpgaErrStr(res));
		goto error;
	}
	printf("waiting for test to complete...\n");

	do {
		++polls;
		res = fpgaReadMMIO64(n5010->handle, 0, n5010->base + ESRAM_TEST_STAT, &stat);
		if (res != FPGA_OK) {
			fprintf(stderr, "failed to read stat: %s\n", fpgaErrStr(res));
			goto error;
		}
		pass	= ESRAM_TEST_PASS(stat);
		fail	= ESRAM_TEST_FAIL(stat);
		timeout = ESRAM_TEST_TIMEOUT(stat);


		if (n5010->debug) {
			printf("pass (16x eSRAM channels)   : 0x%04x\n", pass);
			printf("fail (16x eSRAM channels)   : 0x%04x\n", fail);
			printf("timeout (16x eSRAM channels): 0x%04x\n", timeout);
		}
		nanosleep(&ts, NULL);

	} while (((pass | fail | timeout) != ESRAM_FM) && polls < MAX_POLLS);

	if (polls == MAX_POLLS) {
		fprintf(stderr, "Error: Test failed FPGA not returning result within time\n");
		res = FPGA_EXCEPTION;
	}

	if (fail != 0x0) {
		fprintf(stderr, "Error: Test failed on the following channels: 0x%04x\n", fail);
		res = FPGA_EXCEPTION;
	}

	if (timeout != 0x0) {
		fprintf(stderr, "Error: Test timed out on the following channels: 0x%04x\n", timeout);
		res = FPGA_EXCEPTION;
	}

	if (pass != ESRAM_FM) {
		fprintf(stderr, "Error: Test did not pass on all channels: 0x%04x\n", pass);
		res = FPGA_EXCEPTION;
	}

	if (res != FPGA_OK)
		goto error;

error:
	return res;
}

static fpga_result fpga_test_qdr(struct n5010 *n5010)
{
	struct timespec ts = {.tv_sec = 0, .tv_nsec = 100000000};
	uint64_t stat = 0;
	uint8_t  pass = 0;
	uint8_t  fail = 0;
	uint8_t timeout = 0;
	fpga_result res;
	uint32_t polls = 0;

	printf("starting QDR read/write test\n");

	res = fpgaReadMMIO64(n5010->handle, 0, n5010->base + QDR_TEST_STAT, &stat);
	if (res != FPGA_OK || stat != 0) {
		fprintf(stderr, "FPGA not ready for test, status: 0x%016jx\n", stat);
		if (res == FPGA_OK)
			res = FPGA_EXCEPTION;
		goto error;
	}

	res = fpgaWriteMMIO64(n5010->handle, 0, n5010->base + QDR_TEST_CTRL, 1);
	if (res != FPGA_OK) {
		fprintf(stderr, "failed to start test: %s\n", fpgaErrStr(res));
		goto error;
	}
	printf("waiting for test to complete...\n");

	do {
		++polls;
		res = fpgaReadMMIO64(n5010->handle, 0, n5010->base + QDR_TEST_STAT, &stat);
		if (res != FPGA_OK) {
			fprintf(stderr, "failed to read stat: %s\n", fpgaErrStr(res));
			goto error;
		}
		pass	= QDR_TEST_PASS(stat);
		fail	= QDR_TEST_FAIL(stat);
		timeout = QDR_TEST_TIMEOUT(stat);


		if (n5010->debug) {
			printf("pass (8x QDR channels)   : 0x%02x\n", pass);
			printf("fail (8x QDR channels)   : 0x%02x\n", fail);
			printf("timeout (8x QDR channels): 0x%02x\n", timeout);
		}
		nanosleep(&ts, NULL);

	} while (((pass | fail | timeout) != QDR_FM) && polls < MAX_POLLS);

	if (polls == MAX_POLLS) {
		fprintf(stderr, "Error: Test failed FPGA not returning result within time\n");
		res = FPGA_EXCEPTION;
	}

	if (fail != 0x0) {
		fprintf(stderr, "Error: Test failed on the following channels: 0x%02x\n", fail);
		res = FPGA_EXCEPTION;
	}

	if (timeout != 0x0) {
		fprintf(stderr, "Error: Test timed out on the following channels: 0x%02x\n", timeout);
		res = FPGA_EXCEPTION;
	}

	if (pass != QDR_FM) {
		fprintf(stderr, "Error: Test did not pass on all channels: 0x%02x\n", pass);
		res = FPGA_EXCEPTION;
	}

	if (res != FPGA_OK)
		goto error;

error:
	return res;
}

static fpga_result fpga_test_hbm(struct n5010 *n5010)
{
	struct timespec ts = {.tv_sec = 0, .tv_nsec = 100000000};
	uint64_t pass = 0;
	uint64_t fail = 0;
	uint64_t timeout = 0;
	fpga_result res;

	printf("starting HBM read/write test\n");

	res = fpga_start(n5010, HBM_TEST_CTRL, 32);
	if (res != FPGA_OK)
		goto error;

	printf("waiting for test to complete...\n");

	while (pass == 0 && fail == 0 && timeout == 0) {
		nanosleep(&ts, NULL);

		res = fpgaReadMMIO64(n5010->handle, 0, n5010->base + HBM_TEST_PASS, &pass);
		if (res != FPGA_OK) {
			fprintf(stderr, "failed to read pass: %s\n", fpgaErrStr(res));
			goto error;
		}

		res = fpgaReadMMIO64(n5010->handle, 0, n5010->base + HBM_TEST_FAIL, &fail);
		if (res != FPGA_OK) {
			fprintf(stderr, "failed to read fail: %s\n", fpgaErrStr(res));
			goto error;
		}

		res = fpgaReadMMIO64(n5010->handle, 0, n5010->base + HBM_TEST_TIMEOUT, &timeout);
		if (res != FPGA_OK) {
			fprintf(stderr, "failed to read timeout: %s\n", fpgaErrStr(res));
			goto error;
		}

		if (n5010->debug) {
			printf("reg: 0x%04x (32x HBM channels, pass),    val: 0x%016jx\n", HBM_TEST_PASS, pass);
			printf("reg: 0x%04x (32x HBM channels, fail),    val: 0x%016jx\n", HBM_TEST_FAIL, fail);
			printf("reg: 0x%04x (32x HBM channels, timeout), val: 0x%016jx\n", HBM_TEST_TIMEOUT, timeout);
		}
	}

	if (fail != 0x0) {
		fprintf(stderr, "Error: Test failed on the following channels: 0x%016jx\n", fail);
		res = FPGA_EXCEPTION;
	}

	if (timeout != 0x0) {
		fprintf(stderr, "Error: Test timed out on the following channels: 0x%016jx\n", timeout);
		res = FPGA_EXCEPTION;
	}

	if (pass != 0xffffffff) {
		fprintf(stderr, "Error: Test did not pass on all channels: 0x%016jx\n", pass);
		res = FPGA_EXCEPTION;
	}

	if (res != FPGA_OK)
		goto error;

error:
	return res;
}

static void print_usage(FILE *f)
{
	fprintf(f,
		"usage: %s [<options>]\n"
		"\n"
		"options:\n"
		"  [-S <segment>] [-B <bus>] [-D <device>] [-F <function>]\n"
		"  -S, --segment   pci segment to look up device from\n"
		"  -B, --bus       pci bus to look up device from\n"
		"  -D, --device    pci device number to look up\n"
		"  -F  --function  pci function to look up\n"
		"  -h  --help      print this help\n"
		"  -g  --guid      uuid of accelerator to open\n"
		"  -m  --mode      test mode to execute. Known modes:\n"
		"                  ddr-directed, ddr-prbs, hbm, esram, qdr\n"
		"  -d  --debug     enable debug print of register values\n"
		"  -s  --shared    open FPGA connection in shared mode\n"
		"\n",
		program_invocation_short_name);
}

static bool parse_mode(struct n5010 *n5010, const char *mode)
{
	const struct n5010_test *t;
	size_t count = sizeof(n5010_test) / sizeof(*n5010_test);
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

static fpga_result parse_args(int argc, char **argv, struct n5010 *n5010)
{
	struct option options[] = {
		{ "help",  no_argument,       NULL, 'h' },
		{ "guid",  required_argument, NULL, 'g' },
		{ "mode",  required_argument, NULL, 'm' },
		{ "debug", no_argument,       NULL, 'd' },
		{ "shared",no_argument,       NULL, 's' },
		{ NULL,    0,                 NULL, 0   },
	};

	fpga_result res;
	int c;

	res = fpgaGetProperties(NULL, &n5010->filter);
	if (res != FPGA_OK) {
		fprintf(stderr, "failed to get properties: %s\n", fpgaErrStr(res));
		return res;
	}

	if (opae_set_properties_from_args(n5010->filter, &res, &argc, argv)) {
		fprintf(stderr, "failed to parse device args\n");
		return FPGA_EXCEPTION;
	} else if (res) {
		fprintf(stderr, "failed to set device properties: %s\n", fpgaErrStr(res));
		return res;
	}

	// Set default connection mode
	n5010->open_mode = 0;

	while (1) {
		c = getopt_long_only(argc, argv, "hg:m:d", options, NULL);
		if (c == -1)
			break;

		switch (c) {
		case 'h':
			print_usage(stdout);
			fpga_close(n5010);
			exit(EXIT_SUCCESS);
		case 'g':
			if (uuid_parse(optarg, n5010->guid) < 0) {
				fprintf(stderr, "unparsable uuid: '%s'\n", optarg);
				return FPGA_EXCEPTION;
			}
			break;
		case 'm':
			if (!parse_mode(n5010, optarg))
				return FPGA_EXCEPTION;
			break;
		case 'd':
			n5010->debug = true;
			break;
		case 's':
			n5010->open_mode = FPGA_OPEN_SHARED;
			break;
		case '?':
		default:
			return FPGA_EXCEPTION;
		}
	}

	if (uuid_is_null(n5010->guid)) {
		fprintf(stderr, "no guid given\n");
		return FPGA_EXCEPTION;
	}

	return FPGA_OK;
}

int main(int argc, char **argv)
{
	struct n5010 n5010 = {.test = n5010_test};
	fpga_result res;

	res = parse_args(argc, argv, &n5010);
	if (res != FPGA_OK)
		goto error;

	res = fpga_open(&n5010);
	if (res != FPGA_OK)
		goto error;

	res = fpga_base(&n5010);
	if (res != FPGA_OK)
		goto error;

	fpga_dump(&n5010, 0, 3);

	res = n5010.test->func(&n5010);
	if (res != FPGA_OK)
		goto error;

	printf("passed\n");

error:
	fpga_close(&n5010);

	return res == FPGA_OK ? EXIT_SUCCESS : res;
}

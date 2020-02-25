// Copyright(c) 2017-2020, Intel Corporation
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

/**
 * @file object_api.c
 * @brief A code sample illustrates the basic usage of the OPAE C Object API.
 *
 *
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H
#include <opae/fpga.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>

void print_err(const char *s, fpga_result res)
{
	fprintf(stderr, "Error %s: %s\n", s, fpgaErrStr(res));
}

#define ON_ERR_GOTO(res, label, desc)                                          \
	do {                                                                   \
		if ((res) != FPGA_OK) {                                        \
			print_err((desc), (res));                              \
			goto label;                                            \
		}                                                              \
	} while (0)


typedef fpga_result (*destroy_f)(void *p);
#define MAX_CLEANUP 100
struct {
	destroy_f fn;
	void *param;
} cleanup[MAX_CLEANUP];

static int cleanup_size = 0;

#define ADD_TO_CLEANUP(func, p)                                                \
	if (cleanup_size < MAX_CLEANUP) {                                      \
		cleanup[cleanup_size].fn = (destroy_f)func;                    \
		cleanup[cleanup_size++].param = p;                             \
	}
#define MAX_TOKENS 4
#define MAX_GROUP_OBJECTS 32
typedef struct {
	const char *name;
	fpga_object object;
	uint64_t value;
	uint64_t delta;
} named_object;

typedef struct {
	const char *name;
	fpga_token token;
	fpga_object object;
	uint8_t bus;
	uint8_t device;
	uint8_t function;
	named_object objects[MAX_GROUP_OBJECTS];
	size_t count;
} metric_group;

typedef struct {
	fpga_token token;
	metric_group *groups;
	size_t count;
	fpga_object clock;
} token_group;

struct config {
	int bus;
	float interval_sec;
} options = { -1, 1.0};

metric_group *init_metric_group(fpga_token token, const char *name,
				metric_group *group)
{
	fpga_properties props;
	fpga_result res;

	group->token = token;
	group->name = name;
	group->count = 0;
	if (fpgaGetProperties(token, &props) == FPGA_OK) {
		if ((res = fpgaPropertiesGetBus(props, &group->bus))
		    != FPGA_OK) {
			print_err("error reading bus", res);
		}
		if ((res = fpgaPropertiesGetDevice(props, &group->device))
		    != FPGA_OK) {
			print_err("error reading device", res);
		}
		if ((res = fpgaPropertiesGetFunction(props, &group->function))
		    != FPGA_OK) {
			print_err("error reading function", res);
		}
	}
	fpgaDestroyProperties(&props);
	if (fpgaTokenGetObject(token, name, &group->object, FPGA_OBJECT_GLOB)
	    == FPGA_OK) {
		return group;
	}
	return NULL;
}


fpga_result add_clock(token_group *t_group)
{
	fpga_result res = fpgaTokenGetObject(t_group->token, "*perf/clock",
					     &t_group->clock, FPGA_OBJECT_GLOB);
	if (res == FPGA_OK) {
		ADD_TO_CLEANUP(fpgaDestroyObject, &t_group->clock);
	}

	return res;
}

fpga_result add_counter(metric_group *group, const char *name)
{
	fpga_result res = FPGA_EXCEPTION;
	size_t count = group->count;
	res = fpgaObjectGetObject(group->object, name,
				  &group->objects[count].object,
				  FPGA_OBJECT_GLOB);
	if (res == FPGA_OK) {
		group->objects[count].value = 0;
		group->objects[count].name = name;
		group->count++;
		ADD_TO_CLEANUP(fpgaDestroyObject, &group->objects[count].object);

	} else {
		group->objects[count].object = NULL;
	}
	return res;
}

void print_counters(fpga_object clock, metric_group *group)
{
	size_t count = group->count;
	size_t i;
	uint64_t value = 0, clock_value = 0;
	fpga_result res = fpgaObjectRead64(clock, &clock_value, FPGA_OBJECT_SYNC);
	if (res != FPGA_OK) {
		print_err("Error reading clock", res);
		return;
	}
	for (i = 0; i < count; ++i) {
		res = fpgaObjectRead64(group->objects[i].object, &value,
				       FPGA_OBJECT_SYNC);
		if (res == FPGA_OK) {
			group->objects[i].delta =
				value - group->objects[i].value;
			group->objects[i].value = value;
		} else {
			print_err("error reading counter", res);
		}
	}
	printf("device: %02x:%02x.%1d clock %lu: \n\t", group->bus,
	       group->device, group->function, clock_value);
	for (i = 0; i < count; ++i) {
		if (group->objects[i].delta > 0)
			printf("%16s : %10lu, ", group->objects[i].name,
			       group->objects[i].delta);
	}

	printf("\n");
}


#define GETOPT_STRING "B:v"
fpga_result parse_args(int argc, char *argv[])
{
	struct option longopts[] = {
		{"bus",     required_argument, NULL, 'B'},
		{"version", no_argument,       NULL, 'v'},
		{NULL, 0, NULL, 0},
	};
	int getopt_ret;
	int option_index;
	char *endptr = NULL;

	while (-1 != (getopt_ret = getopt_long(argc, argv, GETOPT_STRING,
					       longopts, &option_index))) {
		const char *tmp_optarg = optarg;
		/* Checks to see if optarg is null and if not it goes to value
		 * of optarg */
		if ((optarg) && ('=' == *tmp_optarg)) {
			++tmp_optarg;
		}

		switch (getopt_ret) {
		case 'B': /* bus */
			if (NULL == tmp_optarg) {
				return FPGA_EXCEPTION;
			}
			endptr = NULL;
			options.bus = (int)strtoul(tmp_optarg, &endptr, 0);
			if (endptr != tmp_optarg + strnlen(tmp_optarg, 100)) {
				fprintf(stderr, "invalid bus: %s\n",
					tmp_optarg);
				return FPGA_EXCEPTION;
			}
			break;

		case 'v': /* version */
			printf("object_api %s %s%s\n",
			       OPAE_VERSION,
			       OPAE_GIT_COMMIT_HASH,
			       OPAE_GIT_SRC_TREE_DIRTY ? "*":"");
			return -1;

		default: /* invalid option */
			fprintf(stderr, "Invalid cmdline option \n");
			return FPGA_EXCEPTION;
		}
	}

	return FPGA_OK;
}

#define USEC_TO_SEC 1000000

int main(int argc, char *argv[])
{
	fpga_properties filter;
	fpga_token tokens[MAX_TOKENS];
	fpga_result res = FPGA_OK;
	int exit_code = -1;
	uint32_t num_matches = 0;
	uint32_t i = 0, j = 0;
	token_group *metrics = NULL;

	if ((res = parse_args(argc, argv)) != FPGA_OK) {
		if((int)res > 0)
			print_err("error parsing arguments", res);
		return -1;
	}

	// create a new filter of type ACCELERATOR
	res = fpgaGetProperties(NULL, &filter);
	ON_ERR_GOTO(res, out, "Creating fpga_properties");
	ADD_TO_CLEANUP(fpgaDestroyProperties, &filter);

	// set filter to accelerator object
	res = fpgaPropertiesSetObjectType(filter, FPGA_DEVICE);
	ON_ERR_GOTO(res, out_free, "Setting ObjectType");

	if (options.bus >= 0) {
		res = fpgaPropertiesSetBus(filter, (uint8_t)options.bus);
		ON_ERR_GOTO(res, out_free, "Setting bus");
	}

	res = fpgaEnumerate(&filter, 1, tokens, MAX_TOKENS, &num_matches);
	ON_ERR_GOTO(res, out_free, "Enumerating for properties");
	for (i = 0; i < num_matches; ++i) {
		ADD_TO_CLEANUP(fpgaDestroyToken, &tokens[i]);
	}

	metrics = calloc(num_matches, sizeof(token_group));
	if (metrics == NULL) {
		fprintf(stderr, "Could not allocate array for metrics\n");
		goto out_free;
	}

	for (i = 0; i < num_matches; ++i) {
		metrics[i].token = tokens[i];
		add_clock(&metrics[i]);
		metrics[i].groups = calloc(3, sizeof(metric_group));
		metrics[i].count = 1;
		metric_group *g_fabric = init_metric_group(
			tokens[i], "*perf/fabric", &metrics[i].groups[0]);
		if (!g_fabric)
			goto out_free;
		ADD_TO_CLEANUP(fpgaDestroyObject, &(g_fabric->object))
		add_counter(g_fabric, "mmio_write");
		add_counter(g_fabric, "mmio_read");
	}
	int count = 10;
	while (--count > 0) {
		for (i = 0; i < num_matches; ++i) {
			for (j = 0; j < metrics[i].count; ++j) {
				print_counters(metrics[i].clock,
					       &metrics[i].groups[j]);
			}
			usleep((useconds_t)(options.interval_sec
					    * USEC_TO_SEC));
		}
	}

	exit_code = 0;

out_free:

	while (cleanup_size-- > 0) {
		if ((res = cleanup[cleanup_size].fn(
			     cleanup[cleanup_size].param))
		    != FPGA_OK) {
			print_err("Error destroying structure: ", res);
		}
	}


	if (metrics) {

		for (i = 0; i < num_matches; ++i) {
			if (metrics[i].groups)
				free(metrics[i].groups);
		}
		free(metrics);
	}
out:
	return exit_code;
}

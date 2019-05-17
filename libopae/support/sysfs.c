// Copyright(c) 2019, Intel Corporation
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

#include "sysfs.h"
#include <dirent.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "safe_string/safe_string.h"

#define PCIE_LOC_PATTERN                                                       \
	"([0-9a-fA-F]{4}):([0-9a-fA-F]{2}):([0-9]{2})\\.([0-9])"
#define PCIE_LOC_PATTERN_GROUPS 5
#define DEVICES_PCI_PATTERN "pci([0-9a-fA-F]{4}):([0-9a-fA-F]{2})"
#define DEVICES_PCI_PATTERN_GROUPS 3


#define PARSE_MATCH_INT(_p, _m, _v, _b, _l)                                    \
	do {                                                                   \
		errno = 0;                                                     \
		_v = strtoul(_p + _m.rm_so, NULL, _b);                         \
		if (errno) {                                                   \
			fprintf(stderr, "error parsing int");                  \
			goto _l;                                               \
		}                                                              \
	} while (0)

#define PCI_DOMAIN_MAX 65536
#define PCI_BUS_MAX 256
#define PCI_DEVICE_MAX 32
#define PCI_FUNCTION_MAX 8

#define SBDF_MAX 16

static int no_dots(const struct dirent *de)
{
	return de->d_name[0] != '.';
}

typedef struct _segment {
	pci_node *nodes[PCI_BUS_MAX][PCI_DEVICE_MAX][PCI_FUNCTION_MAX];
} pcie_segment;

pcie_segment *_segments = NULL;

int read_attribute(const char *root, const char *attr, char *buffer)
{
	char path[PATH_MAX] = {0};
	FILE *fattr = NULL;
	size_t bytes_read = 0;
	if (snprintf_s_ss(path, sizeof(path), "%s/%s", root, attr) < 0) {
		fprintf(stderr, "error formatting path\n");
		return 0;
	}

	fattr = fopen(path, "r");
	if (!fattr) {
		fprintf(stderr, "error opening attribute: %s\n", path);
		return 0;
	}

	bytes_read = fread(buffer, 1, sizeof(buffer), fattr);
	if (bytes_read <= 0 || ferror(fattr)) {
		fprintf(stderr, "error reading attribute; %s\n", path);
		goto out_close;
	}


out_close:
	fclose(fattr);
	return bytes_read;
}

int parse_attribute32(const char *root, const char *attr, uint32_t *value)
{
	const uint64_t pg_size = (uint64_t)sysconf(_SC_PAGE_SIZE);
	char buffer[pg_size];
	if (read_attribute(root, attr, buffer)) {
		*value = strtoul(buffer, NULL, 0);
		return 0;
	}

	return 1;
}

int parse_attribute64(const char *root, const char *attr, uint64_t *value)
{
	const uint64_t pg_size = (uint64_t)sysconf(_SC_PAGE_SIZE);
	char buffer[pg_size];
	if (read_attribute(root, attr, buffer)) {
		*value = strtoull(buffer, NULL, 0);
		return 0;
	}

	return 1;
}


pcie_segment *pcie_segment_get(uint32_t num) {
	size_t cur_size = _segments == NULL ? 0 : sizeof(_segments)/sizeof(pcie_segment);
	if (num >= cur_size) {
		pcie_segment *tmp = _segments;
		_segments = calloc((num+1)*2, sizeof(pcie_segment));
		if (!_segments) {
			fprintf(stderr, "error allocating pcie_segment array\n");
			return NULL;
		}
		if (!cur_size) {
			memcpy_s(_segments, sizeof(_segments), tmp, sizeof(tmp));
		}
	}
	return &_segments[num];
}

const pci_node *pci_nodes_get(const char *bus_path)
{
	const char *ptr = bus_path;
	pci_node *node = NULL;
	regex_t re;
	regmatch_t matches[PCIE_LOC_PATTERN_GROUPS];
	char sbdf[SBDF_MAX];
	uint32_t segment = 0;
	uint8_t bus = 0;
	uint8_t device = 0;
	uint8_t function = 0;


	if (regcomp(&re, PCIE_LOC_PATTERN, REG_EXTENDED | REG_ICASE)) {
		fprintf(stderr, "error compiling regex: %s\n",
			PCIE_LOC_PATTERN);
		return NULL;
	}
	while (!regexec(&re, ptr, PCIE_LOC_PATTERN_GROUPS, matches, 0)) {
		pci_node *parent = node;
		PARSE_MATCH_INT(ptr, matches[1], segment, 16, out_free);
		PARSE_MATCH_INT(ptr, matches[2], bus, 16, out_free);
		PARSE_MATCH_INT(ptr, matches[3], device, 16, out_free);
		PARSE_MATCH_INT(ptr, matches[4], function, 10, out_free);
		pcie_segment *seg = pcie_segment_get(segment);
		if (!seg) {
			goto out_free;
		}

		if (!seg->nodes[bus][device][function]) {
			node = malloc(sizeof(pci_node));
			if (!node) {
				fprintf(stderr,
					"error allocating pci_node struct\n");
				goto out_free;
			}
			strncpy_s(sbdf, SBDF_MAX, ptr + matches[0].rm_so,
				  matches[0].rm_eo - matches[0].rm_so);
			snprintf_s_s(node->pci_bus_path,
				     sizeof(node->pci_bus_path),
				     "/sys/bus/pci/devices/%s", sbdf);
			node->segment = segment;
			node->bus = bus;
			node->device = device;
			node->function = function;
			if (parse_attribute32(node->pci_bus_path, "vendor",
					      &node->vendor_id)
			    || parse_attribute32(node->pci_bus_path, "device",
						 &node->device_id)) {
				fprintf(stderr, "error reading vendor/device\n");
			}
			seg->nodes[bus][device][function] = node;
		} else {
			node = seg->nodes[bus][device][function];
		}

		node->parent = parent;
		ptr += matches[4].rm_eo;
	}
out_free:
	regfree(&re);
	return node;
}

size_t sysfs_enum_class(const char *class_name, sysfs_object *objects,
			size_t max_objects)
{
	struct dirent **namelist = NULL;
	char pci_bus_path[PATH_MAX] = {0};
	char path[SYSFS_PATH_MAX] = {0};
	const pci_node *node = NULL;

	int n = 0;
	size_t total_count = 0, count = 0;
	if (!class_name) {
		fprintf(stderr, "class name is null\n");
		return 0;
	}


	snprintf_s_s(path, sizeof(path), "/sys/class/%s", class_name);
	total_count = n = scandir(path, &namelist, no_dots, alphasort);
	if (n < 0) {
		fprintf(stderr, "error scanning directory, %s: %s\n", path,
			strerror(errno));
		return 0;
	}

	while (n--) {
		if (count < max_objects && objects) {
			if (!snprintf_s_ss(objects[n].sysfs_path,
					   SYSFS_PATH_MAX, "%s/%s", path,
					   namelist[n]->d_name)) {
				fprintf(stderr,
					"could not format path for sysfs object: %s\n",
					namelist[n]->d_name);
			} else if (realpath(objects[n].sysfs_path,
					    pci_bus_path)) {
				node = pci_nodes_get(pci_bus_path);
				if (node) {
					objects[n].pci_object = node;
					count++;
				} else {
					objects[n].pci_object = NULL;
					fprintf(stderr,
						"could not get pci_node for sysfs object: %s\n",
						namelist[n]->d_name);
				}
			} else {
				fprintf(stderr,
					"could not get realpath for sysfs object: %s\n",
					namelist[n]->d_name);
			}
		}
		free(namelist[n]);
	}
	if (namelist) {
		free(namelist);
	}
	return max_objects == 0 ? total_count : count;
}

const pci_node *sysfs_root_port(const sysfs_object *obj)
{
	if (!obj) {
		fprintf(stderr, "sysfs_object is NULL\n");
		return NULL;
	}

	const pci_node *port = obj->pci_object;
	while (port->parent) {
		port = port->parent;
	}
	return port;
}

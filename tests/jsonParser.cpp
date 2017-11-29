/*++

  INTEL CONFIDENTIAL
  Copyright 2016 - 2017 Intel Corporation

  The source code contained or described  herein and all documents related to
  the  source  code  ("Material")  are  owned  by  Intel  Corporation  or its
  suppliers  or  licensors.  Title   to  the  Material   remains  with  Intel
  Corporation or  its suppliers  and licensors.  The Material  contains trade
  secrets  and  proprietary  and  confidential  information  of Intel  or its
  suppliers and licensors.  The Material is protected  by worldwide copyright
  and trade secret laws and treaty provisions. No part of the Material may be
  used,   copied,   reproduced,   modified,   published,   uploaded,  posted,
  transmitted,  distributed, or  disclosed in  any way  without Intel's prior
  express written permission.

  No license under any patent, copyright,  trade secret or other intellectual
  property  right  is  granted to  or conferred  upon  you by  disclosure  or
  delivery of the  Materials, either  expressly, by  implication, inducement,
  estoppel or otherwise. Any license  under such intellectual property rights
  must be express and approved by Intel in writing.

  --*/

#include <fstream>
#include <iostream>
#include <cstdio>
#include <cassert>
#include <cstring>
#include <map>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <json-c/json.h>
#include <json-c/json_object.h>

#include "common_test.h"
#include "signal.h"

using namespace common_test;
using namespace std;

#define MAX_JSON_BUFFER 4096

namespace common_test {

// callee frees file_buffer
char* getConfigurationFile(const char* path) {
	char* file_buffer = NULL;
	signed file_len = 0;
	file_buffer = (char*)calloc(MAX_JSON_BUFFER, sizeof(char));
	assert(file_buffer);
	memset(file_buffer, '0', MAX_JSON_BUFFER);

	// json file path
	string jsonfilename(path);

	ifstream jsonfile(jsonfilename.c_str(), ios::in);
	if (!jsonfile.good()) {
		cout << "UNABLE TO PARSE JSON FILE" << endl;
		free(file_buffer);
		return NULL;
	}

	jsonfile.seekg(0, ios::end);
	file_len = jsonfile.tellg();
	assert(file_len > 0);

	jsonfile.seekg(0, ios::beg);

	jsonfile.read(file_buffer, file_len);

	return file_buffer;
}

void fetchConfiguration(const char* path) {
	cout << "fetching configuration from:  " << path << endl;

	char* json_file_data = getConfigurationFile(path);
	assert(json_file_data);

	json_tokener* tok = json_tokener_new();

	json_object* root = NULL;
	json_object* values = NULL;
	json_object* value = NULL;

	enum json_tokener_error json_error = json_tokener_continue;

	do {
		root = json_tokener_parse_ex(tok, json_file_data, strlen(json_file_data));

		json_object_object_get_ex(root, "config_elements", &values);
		assert(values);

		json_object_object_get_ex(values, "bitstream0", &value);
		assert(value);
		memcpy_s(config_map[BITSTREAM_MODE0], MAX_PATH,
			 json_object_get_string(value),
			 strlen(json_object_get_string(value)));

		json_object_object_get_ex(values, "installpath", &value);
		assert(value);
		memcpy_s(config_map[OPAE_INSTALL_PATH], MAX_PATH,
			 json_object_get_string(value),
			 strlen(json_object_get_string(value)));

		json_object_object_get_ex(values, "num_sockets", &value);
		assert(value);
		GlobalOptions::Instance().NumSockets(
			(unsigned)strtoul(json_object_get_string(value), NULL, 0));
		cout << "Setting num sockets to " << GlobalOptions::Instance().NumSockets()
		     << endl;

	} while ((json_error = json_tokener_get_error(tok)) == json_tokener_continue);

	if (json_error != json_tokener_success) {
		fprintf(stderr, "Error: %s\n", json_tokener_error_desc(json_error));
	}

	json_object_put(root);
	json_tokener_free(tok);
	free(json_file_data);
}

}  // end namespace

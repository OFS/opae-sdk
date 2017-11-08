#include <assert.h>
#include <json-c/json.h>
#include <json-c/json_object.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <map>
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

    json_object_object_get_ex(values, "bitstream3", &value);
    assert(value);
    memcpy_s(config_map[BITSTREAM_MODE3], MAX_PATH,
             json_object_get_string(value),
             strlen(json_object_get_string(value)));

    json_object_object_get_ex(values, "bitstream7", &value);
    assert(value);
    memcpy_s(config_map[BITSTREAM_MODE7], MAX_PATH,
             json_object_get_string(value),
             strlen(json_object_get_string(value)));

    json_object_object_get_ex(values, "bitstream_mmio", &value);
    assert(value);
    memcpy_s(config_map[BITSTREAM_MMIO], MAX_PATH,
             json_object_get_string(value),
             strlen(json_object_get_string(value)));

    json_object_object_get_ex(values, "bitstream_sigtap", &value);
    assert(value);
    memcpy_s(config_map[BITSTREAM_SIGTAP], MAX_PATH,
             json_object_get_string(value),
             strlen(json_object_get_string(value)));

    json_object_object_get_ex(values, "bitstream_sw_pr_07", &value);
    assert(value);
    memcpy_s(config_map[BITSTREAM_PR07], MAX_PATH,
             json_object_get_string(value),
             strlen(json_object_get_string(value)));

    json_object_object_get_ex(values, "bitstream_sw_pr_08", &value);
    assert(value);
    memcpy_s(config_map[BITSTREAM_PR08], MAX_PATH,
             json_object_get_string(value),
             strlen(json_object_get_string(value)));

    json_object_object_get_ex(values, "bitstream_sw_pr_09", &value);
    assert(value);
    memcpy_s(config_map[BITSTREAM_PR09], MAX_PATH,
             json_object_get_string(value),
             strlen(json_object_get_string(value)));

    json_object_object_get_ex(values, "bitstream_sw_pr_10", &value);
    assert(value);
    memcpy_s(config_map[BITSTREAM_PR10], MAX_PATH,
             json_object_get_string(value),
             strlen(json_object_get_string(value)));

    json_object_object_get_ex(values, "bitstream_sw_pr_18", &value);
    assert(value);
    memcpy_s(config_map[BITSTREAM_PR18], MAX_PATH,
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

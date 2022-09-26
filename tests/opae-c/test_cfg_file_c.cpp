// Copyright(c) 2022, Intel Corporation
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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <stack>
#include <libgen.h>
#include <linux/limits.h>

#include "mock/opae_fixtures.h"
#include "cfg-file.h"

#define MAX_CFG_SIZE (8 * 4096)

#define CFG_PATH_MAX  64
#define HOME_CFG_PATHS 3
#define SYS_CFG_PATHS  2
extern "C" {

extern const char _opae_home_cfg_files[][CFG_PATH_MAX];
extern const char _opae_sys_cfg_files[][CFG_PATH_MAX];

const char *_opae_home_configs[HOME_CFG_PATHS] = {
  _opae_home_cfg_files[0],
  _opae_home_cfg_files[1],
  _opae_home_cfg_files[2]
};

const char *_opae_sys_configs[SYS_CFG_PATHS] = {
  _opae_sys_cfg_files[0],
  _opae_sys_cfg_files[1]
};

int opae_plugin_mgr_initialize(const char *cfg_file);
int opae_plugin_mgr_finalize_all(void);

extern libopae_config_data default_libopae_config_table[];
extern fpgainfo_config_data default_fpgainfo_config_table[];
extern fpgad_config_data default_fpgad_config_table[];
}

using namespace opae::testing;

/**
 * @test       parse_array_err1
 * @brief      Test: parse_json_array
 * @details    When the input json object to parse_json_array<br>
 *             has no element that matches the name parameter,<br>
 *             then the function returns NULL.
 */
TEST(cfg_file, parse_array_err1) {
  const char *json = R"json(
{
}
)json";
  json_tokener_error j_err = json_tokener_success;
  json_object *root;
  int len = 0;

  root = json_tokener_parse_verbose(json, &j_err);
  ASSERT_NE((void *)NULL, root);

  EXPECT_EQ((void *)NULL, parse_json_array(root, "key", &len));

  json_object_put(root);
}

/**
 * @test       parse_array_err2
 * @brief      Test: parse_json_array
 * @details    When the input json object to parse_json_array<br>
 *             has an element that matches the name parameter,<br>
 *             but the element is not an array,<br>
 *             then the function returns NULL.
 */
TEST(cfg_file, parse_array_err2) {
  const char *json = R"json(
{
  "key": "this is not an array"
}
)json";
  json_tokener_error j_err = json_tokener_success;
  json_object *root;
  int len = 0;

  root = json_tokener_parse_verbose(json, &j_err);
  ASSERT_NE((void *)NULL, root);

  EXPECT_EQ((void *)NULL, parse_json_array(root, "key", &len));

  json_object_put(root);
}

/**
 * @test       parse_array
 * @brief      Test: parse_json_array
 * @details    When the input json object to parse_json_array<br>
 *             has an element that matches the name parameter,<br>
 *             and the element is an array,<br>
 *             then the function returns a json_object * for<br>
 *             the parsed element, and stores the array length<br>
 *             in a non-NULL len pointer.
 */
TEST(cfg_file, parse_array) {
  const char *json = R"json(
{
  "arr": [
    { "keya": "vala" },
    { "keyb": "valb" }
  ]
}
)json";
  json_tokener_error j_err = json_tokener_success;
  json_object *root;
  int len = 0;

  root = json_tokener_parse_verbose(json, &j_err);
  ASSERT_NE((void *)NULL, root);

  EXPECT_NE((void *)NULL, parse_json_array(root, "arr", &len));
  EXPECT_EQ(2, len);

  json_object_put(root);
}

/**
 * @test       parse_boolean_err1
 * @brief      Test: parse_json_boolean
 * @details    When the input json object to parse_json_boolean<br>
 *             has no element that matches the name parameter,<br>
 *             then the function returns NULL.
 */
TEST(cfg_file, parse_boolean_err1) {
  const char *json = R"json(
{
}
)json";
  json_tokener_error j_err = json_tokener_success;
  json_object *root;
  bool val = false;

  root = json_tokener_parse_verbose(json, &j_err);
  ASSERT_NE((void *)NULL, root);

  EXPECT_EQ((void *)NULL, parse_json_boolean(root, "key", &val));

  json_object_put(root);
}

/**
 * @test       parse_boolean_err2
 * @brief      Test: parse_json_boolean
 * @details    When the input json object to parse_json_boolean<br>
 *             has an element that matches the name parameter,<br>
 *             but the element is not a boolean,<br>
 *             then the function returns NULL.
 */
TEST(cfg_file, parse_boolean_err2) {
  const char *json = R"json(
{
  "key": "this is not a boolean"
}
)json";
  json_tokener_error j_err = json_tokener_success;
  json_object *root;
  bool val = false;

  root = json_tokener_parse_verbose(json, &j_err);
  ASSERT_NE((void *)NULL, root);

  EXPECT_EQ((void *)NULL, parse_json_boolean(root, "key", &val));

  json_object_put(root);
}

/**
 * @test       parse_boolean
 * @brief      Test: parse_json_boolean
 * @details    When the input json object to parse_json_boolean<br>
 *             has an element that matches the name parameter,<br>
 *             and the element is a boolean,<br>
 *             then the function returns a json_object * for<br>
 *             the parsed element, and stores the boolean value<br>
 *             in a non-NULL value pointer.
 */
TEST(cfg_file, parse_boolean) {
  const char *json = R"json(
{
  "mybool": true
}
)json";
  json_tokener_error j_err = json_tokener_success;
  json_object *root;
  bool val = false;

  root = json_tokener_parse_verbose(json, &j_err);
  ASSERT_NE((void *)NULL, root);

  EXPECT_NE((void *)NULL, parse_json_boolean(root, "mybool", &val));
  EXPECT_EQ(true, val);

  json_object_put(root);
}

/**
 * @test       parse_string_err1
 * @brief      Test: parse_json_string
 * @details    When the input json object to parse_json_string<br>
 *             has no element that matches the name parameter,<br>
 *             then the function returns NULL.
 */
TEST(cfg_file, parse_string_err1) {
  const char *json = R"json(
{
}
)json";
  json_tokener_error j_err = json_tokener_success;
  json_object *root;
  char *val = NULL;

  root = json_tokener_parse_verbose(json, &j_err);
  ASSERT_NE((void *)NULL, root);

  EXPECT_EQ((void *)NULL, parse_json_string(root, "key", &val));

  json_object_put(root);
}

/**
 * @test       parse_string_err2
 * @brief      Test: parse_json_string
 * @details    When the input json object to parse_json_string<br>
 *             has an element that matches the name parameter,<br>
 *             but the element is not a string,<br>
 *             then the function returns NULL.
 */
TEST(cfg_file, parse_string_err2) {
  const char *json = R"json(
{
  "key": true
}
)json";
  json_tokener_error j_err = json_tokener_success;
  json_object *root;
  char *val = NULL;

  root = json_tokener_parse_verbose(json, &j_err);
  ASSERT_NE((void *)NULL, root);

  EXPECT_EQ((void *)NULL, parse_json_string(root, "key", &val));

  json_object_put(root);
}

/**
 * @test       parse_string
 * @brief      Test: parse_json_string
 * @details    When the input json object to parse_json_string<br>
 *             has an element that matches the name parameter,<br>
 *             and the element is a string,<br>
 *             then the function returns a json_object * for<br>
 *             the parsed element, and stores the string value<br>
 *             in a non-NULL value pointer.
 */
TEST(cfg_file, parse_string) {
  const char *json = R"json(
{
  "mystr": "This is a string"
}
)json";
  json_tokener_error j_err = json_tokener_success;
  json_object *root;
  char *val = NULL;

  root = json_tokener_parse_verbose(json, &j_err);
  ASSERT_NE((void *)NULL, root);

  EXPECT_NE((void *)NULL, parse_json_string(root, "mystr", &val));
  EXPECT_STREQ("This is a string", val);

  json_object_put(root);
}

/**
 * @test       string_to_unsigned_0
 * @brief      Test: string_to_unsigned_wildcard
 * @details    When the input s(tring) parameter to string_to_unsigned_wildcard<br>
 *             has a '*' character at index 0,<br>
 *             then the function copies the w(ildcard) value<br>
 *             into a non-NULL u parameter,<br>
 *             and the function returns 0.
 */
TEST(cfg_file, string_to_unsigned_0) {
  const char *input = "*ou812";
  unsigned long u = 0;
  const unsigned long w = 0x1234;

  EXPECT_EQ(0, string_to_unsigned_wildcard(input, &u, w));
  EXPECT_EQ(u, w);
}

/**
 * @test       string_to_unsigned_1
 * @brief      Test: string_to_unsigned_wildcard
 * @details    When the input s(tring) parameter to string_to_unsigned_wildcard<br>
 *             is a valid unsigned integer value,<br>
 *             then the function converts the string to that integer value,<br>
 *             storing it into a non-NULL u(nsigned) parameter;<br>
 *             and the function returns 0.
 */
TEST(cfg_file, string_to_unsigned_1) {
  const char *input = "812";
  unsigned long u = 0;
  const unsigned long w = 0x1234;

  EXPECT_EQ(0, string_to_unsigned_wildcard(input, &u, w));
  EXPECT_EQ(812, u);
}

/**
 * @test       string_to_unsigned_err0
 * @brief      Test: string_to_unsigned_wildcard
 * @details    When the input s(tring) parameter to string_to_unsigned_wildcard<br>
 *             is not a valid unsigned integer value,<br>
 *             and when it does not contain a '*' at index 0,<br>
 *             then the function does not modify the value at u(nsigned),<br>
 *             and the function returns non-zero.
 */
TEST(cfg_file, string_to_unsigned_err0) {
  const char *input = "8v1w2";
  unsigned long u = 0;
  const unsigned long w = 0x1234;

  EXPECT_NE(0, string_to_unsigned_wildcard(input, &u, w));
  EXPECT_EQ(0, u);
}

/**
 * @test       string_to_signed_0
 * @brief      Test: string_to_signed_wildcard
 * @details    When the input s(tring) parameter to string_to_signed_wildcard<br>
 *             has a '*' character at index 0,<br>
 *             then the function copies the w(ildcard) value<br>
 *             into a non-NULL l parameter,<br>
 *             and the function returns 0.
 */
TEST(cfg_file, string_to_signed_0) {
  const char *input = "*-3";
  long l = 0;
  const long w = -7;

  EXPECT_EQ(0, string_to_signed_wildcard(input, &l, w));
  EXPECT_EQ(l, w);
}

/**
 * @test       string_to_signed_1
 * @brief      Test: string_to_signed_wildcard
 * @details    When the input s(tring) parameter to string_to_signed_wildcard<br>
 *             is a valid signed integer value,<br>
 *             then the function converts the string to that integer value,<br>
 *             storing it into a non-NULL l(ong) parameter;<br>
 *             and the function returns 0.
 */
TEST(cfg_file, string_to_signed_1) {
  const char *input = "-3";
  long l = 0;
  const long w = -7;

  EXPECT_EQ(0, string_to_signed_wildcard(input, &l, w));
  EXPECT_EQ(-3, l);
}

/**
 * @test       string_to_signed_err0
 * @brief      Test: string_to_signed_wildcard
 * @details    When the input s(tring) parameter to string_to_signed_wildcard<br>
 *             is not a valid signed integer value,<br>
 *             and when it does not contain a '*' at index 0,<br>
 *             then the function does not modify the value at l(ong),<br>
 *             and the function returns non-zero.
 */
TEST(cfg_file, string_to_signed_err0) {
  const char *input = "-3v1w2";
  long l = 0;
  const long w = -7;

  EXPECT_NE(0, string_to_signed_wildcard(input, &l, w));
  EXPECT_EQ(0, l);
}

const char *dummy_cfg = R"plug(
{
  "configurations": {

    "dummy": {
      "enabled": true,
      "platform": "Unit Test platform",

      "devices": [
        { "name": "dum0", "id": [ "0x8086", "0xbcbd", "*", "*" ] }
      ]

      "opae": {
        "plugin": [
          "enabled": true,
          "module": "libdummy_plugin.so",
          "devices": [ "dum0" ],
          "configuration": {
            "key1": "hello",
            "key2": "plugin",
            "fake_tokens": 99
          }
        ],
        "fpgainfo": [],
        "fpgad": [],
        "rsu": []
      }

    }

  },

  "configs": [
    "dummy"
  ]
}
)plug";

class pluginmgr_cfg_p : public ::testing::TestWithParam<const char *> {
 protected:
  pluginmgr_cfg_p() : buffer_ {0} {}

  virtual void SetUp() override {
    // This parameterized test iterates over the possible config file paths
    // relative to a user's home directory

    // let's build the full path by prepending the parameter with $HOME
    char *home_cstr = getenv("HOME");
    ASSERT_NE(home_cstr, nullptr) << "No home environment found";
    std::string home = home_cstr;
    // the parameter paths start with a '/'
    cfg_file_ = home + std::string(GetParam());
    // copy it to a temporary buffer that we can use dirname with
    std::copy(cfg_file_.begin(), cfg_file_.end(), &buffer_[0]);
    // get the directory name of the file
    cfg_dir_ = dirname(buffer_);
    struct stat st;
    // if the directory doesn't exist, create the entire path
    if (opae_stat(cfg_dir_, &st)) {
      std::string dir = cfg_dir_;
      // find the first '/' after $HOME
      size_t pos = dir.find('/', home.size());
      while (pos != std::string::npos) {
        std::string sub = dir.substr(0, pos);
        // sub is $HOME/<dir1>, then $HOME/<dir1>/<dir2>, ...
        // if this directory doesn't exist, create it
        if (opae_stat(sub.c_str(), &st) && sub != "") {
          ASSERT_EQ(mkdir(sub.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH),
                    0)
              << "Error creating subdirectory (" << sub
              << "}: " << strerror(errno);
          // keep track of directories created
          dirs_.push(sub);
        }
        pos = pos < dir.size() ? dir.find('/', pos + 1) : std::string::npos;
      }
      // finally, we know the entire path didn't exist, create the last
      // directory
      ASSERT_EQ(mkdir(cfg_dir_, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0)
          << "Error creating subdirectory (" << cfg_dir_
          << "}: " << strerror(errno);
      dirs_.push(cfg_dir_);
    }

    if (opae_stat(cfg_file_.c_str(), &st) == 0) {
      unlink(cfg_file_.c_str());
    }

    std::ofstream cfg_stream(cfg_file_);
    cfg_stream.write(dummy_cfg, strlen(dummy_cfg));
    cfg_stream.close();
  }

  virtual void TearDown() override {
    opae_plugin_mgr_finalize_all();
    unlink(cfg_file_.c_str());
    // remove any directories we created in SetUp
    while (!dirs_.empty()) {
      unlink(dirs_.top().c_str());
      dirs_.pop();
    }
  }

  char buffer_[PATH_MAX];
  std::string cfg_file_;
  char *cfg_dir_;
  std::stack<std::string> dirs_;
};

/**
 * @test       find_home_cfg
 * @brief      Test: opae_find_cfg_file
 * @details    Given a valid configuration with one plugin<br>
 *             And a configuration file located in one of three possible paths
 *             in the user's home directory<br>
 *             When I call opae_plugin_mgr_initialize
 *             Then the call is successful<br>
 *             And the number of plugins in the global plugin list is 1
 */
TEST_P(pluginmgr_cfg_p, find_home_cfg) {
  char *cfg_file = opae_find_cfg_file();
  ASSERT_NE((char *)NULL, cfg_file);
  EXPECT_STREQ(cfg_file_.c_str(), cfg_file);
  opae_free(cfg_file);
}

INSTANTIATE_TEST_SUITE_P(pluginmgr_home_cfg, pluginmgr_cfg_p,
                         ::testing::ValuesIn(_opae_home_configs));

/**
 * @test       find_env_cfg_0
 * @brief      Test: opae_find_cfg_file
 * @details    When environment variable LIBOPAE_CFGFILE is set,<br>
 *             but its value is empty,<br>
 *             then the search continues on with the hard-coded paths.
 */
TEST(cfg_file, find_env_cfg_0) {
  char env[64];

  snprintf(env, sizeof(env), "LIBOPAE_CFGFILE=");
  putenv(env);

  char *cfg_file = opae_find_cfg_file();
  EXPECT_EQ((char *)NULL, cfg_file);

  unsetenv("LIBOPAE_CFGFILE");
}

/**
 * @test       find_env_cfg_1
 * @brief      Test: opae_find_cfg_file
 * @details    When environment variable LIBOPAE_CFGFILE is set,<br>
 *             but its value names a non-existent file,<br>
 *             then the search continues on with the hard-coded paths.
 */
TEST(cfg_file, find_env_cfg_1) {
  char env[64];

  snprintf(env, sizeof(env), "LIBOPAE_CFGFILE=/does/not/exist/opae.cfg");
  putenv(env);

  char *cfg_file = opae_find_cfg_file();
  EXPECT_EQ((char *)NULL, cfg_file);

  unsetenv("LIBOPAE_CFGFILE");
}

/**
 * @test       find_env_cfg_2
 * @brief      Test: opae_find_cfg_file
 * @details    When environment variable LIBOPAE_CFGFILE is set,<br>
 *             and its value names a valid file,<br>
 *             then the search completes with that file.
 */
TEST(cfg_file, find_env_cfg_2) {
  char tmpcfg[32];
  char env[64];

  strcpy(tmpcfg, "tmp-XXXXXX.cfg");
  opae_close(mkstemps(tmpcfg, 4));

  std::ofstream cfg_stream(tmpcfg);
  cfg_stream.write(dummy_cfg, strlen(dummy_cfg));
  cfg_stream.close();

  snprintf(env, sizeof(env),
	   "LIBOPAE_CFGFILE=%s", tmpcfg);
  putenv(env);

  char *cfg_file = opae_find_cfg_file();
  EXPECT_NE((char *)NULL, cfg_file);

  char *after_slash = strrchr(cfg_file, '/');
  if (after_slash) {
    EXPECT_STREQ(tmpcfg, after_slash + 1);
  }

  unlink(cfg_file);
  opae_free(cfg_file);
  unsetenv("LIBOPAE_CFGFILE");
}

/**
 * @test       read_cfg0
 * @brief      Test: opae_read_cfg_file
 * @details    When the input config_file_path parameter is NULL<br>
 *             then the function returns a NULL buffer pointer.
 */
TEST(cfg_file, read_cfg0) {
  EXPECT_EQ((char *)NULL, opae_read_cfg_file(NULL));
}

/**
 * @test       read_cfg1
 * @brief      Test: opae_read_cfg_file
 * @details    When the input config_file_path parameter is valid,<br>
 *             but it points to a file that doesn't exist,<br>
 *             then the function returns a NULL buffer pointer.
 */
TEST(cfg_file, read_cfg1) {
  EXPECT_EQ((char *)NULL, opae_read_cfg_file("/does/not/exist/opae.cfg"));
}

/**
 * @test       read_cfg2
 * @brief      Test: opae_read_cfg_file
 * @details    When the input config_file_path parameter is valid,<br>
 *             and when it points to a file that exists,<br>
 *             but the file size exceeds MAX_CFG_SIZE,<br>
 *             then the function returns a NULL buffer pointer.
 */
TEST(cfg_file, read_cfg2) {
  char cfg[PATH_MAX];

  strcpy(cfg, "opae.cfg.XXXXXX");
  opae_close(mkstemp(cfg));

  FILE *fp = opae_fopen(cfg, "wb");
  char write_buff[MAX_CFG_SIZE + 1]; // uninitialized (doesn't matter)
  fwrite(write_buff, 1, MAX_CFG_SIZE+1, fp); 
  opae_fclose(fp);

  EXPECT_EQ((char *)NULL, opae_read_cfg_file(cfg));

  unlink(cfg);
}

class cfg_mock_c_p : public opae_base_p<> {
 protected:

  virtual void SetUp() override {
    opae_base_p<>::SetUp();

    strcpy(cfg_, "opae.cfg.XXXXXX");
    opae_close(mkstemp(cfg_));

    FILE *fp = opae_fopen(cfg_, "w");
    fwrite(dummy_cfg, 1, strlen(dummy_cfg), fp);
    opae_fclose(fp);
  }

  virtual void TearDown() override {
    unlink(cfg_);
    cfg_[0] = '\0';

    opae_base_p<>::TearDown();
  }

  char cfg_[PATH_MAX];
};

/**
 * @test       read_cfg3
 * @brief      Test: opae_read_cfg_file
 * @details    When the input config_file_path parameter is valid,<br>
 *             and when it points to a file that exists,<br>
 *             but malloc fails,<br>
 *             then the function returns a NULL buffer pointer.
 */
TEST_P(cfg_mock_c_p, read_cfg3) {
  system_->invalidate_malloc(0, "opae_read_cfg_file");
  EXPECT_EQ((char *)NULL, opae_read_cfg_file(cfg_));
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(cfg_mock_c_p);
INSTANTIATE_TEST_SUITE_P(cfg_c, cfg_mock_c_p,
                         ::testing::ValuesIn(test_platform::mock_platforms({
                                                                             "dfl-n6000-sku0",
                                                                             "dfl-n6000-sku1",
                                                                             "dfl-c6100"
                                                                           })));

/**
 * @test       parse_device_id0
 * @brief      Test: opae_parse_device_id
 * @details    When the j_id parameter does not point to a JSON array,<br>
 *             then the function returns non-zero.<br>
 */
TEST(cfg_file, parse_device_id0) {
  const char *json = R"json(
{
  "id": "this is not an array"
}
)json";
  json_tokener_error j_err = json_tokener_success;
  json_object *root;
  json_object *j_id;
  opae_pci_device dev;

  root = json_tokener_parse_verbose(json, &j_err);
  ASSERT_NE((void *)NULL, root);

  j_id = parse_json_array(root, "id", NULL);

  EXPECT_NE(0, opae_parse_device_id(j_id, &dev));

  json_object_put(root);
}

/**
 * @test       parse_device_id1
 * @brief      Test: opae_parse_device_id
 * @details    When the j_id parameter points to a JSON array,<br>
 *             but the JSON array has fewer than 4 entries,<br>
 *             then the function returns non-zero.<br>
 */
TEST(cfg_file, parse_device_id1) {
  const char *json = R"json(
{
  "id": [ "0x8086", "0xbcbd", "0" ]
}
)json";
  json_tokener_error j_err = json_tokener_success;
  json_object *root;
  json_object *j_id;
  opae_pci_device dev;

  root = json_tokener_parse_verbose(json, &j_err);
  ASSERT_NE((void *)NULL, root);

  j_id = parse_json_array(root, "id", NULL);

  EXPECT_NE(0, opae_parse_device_id(j_id, &dev));

  json_object_put(root);
}

/**
 * @test       parse_device_id2
 * @brief      Test: opae_parse_device_id
 * @details    When the j_id parameter points to a JSON array,<br>
 *             and the JSON array has at least 4 entries,<br>
 *             but at least one of the entries is not a string,<br>
 *             then the function returns non-zero.<br>
 */
TEST(cfg_file, parse_device_id2) {
  const char *json = R"json(
{
  "id": [ "0x8086", "0xbcbd", "0", 0 ]
}
)json";
  json_tokener_error j_err = json_tokener_success;
  json_object *root;
  json_object *j_id;
  opae_pci_device dev;

  root = json_tokener_parse_verbose(json, &j_err);
  ASSERT_NE((void *)NULL, root);

  j_id = parse_json_array(root, "id", NULL);

  EXPECT_NE(0, opae_parse_device_id(j_id, &dev));

  json_object_put(root);
}

/**
 * @test       parse_device_id3
 * @brief      Test: opae_parse_device_id
 * @details    When the j_id parameter points to a JSON array,<br>
 *             and the JSON array has at least 4 entries,<br>
 *             and each of the entries is a string,<br>
 *             but the VID field is not the string representation of an integer,<br>
 *             then the function returns non-zero.<br>
 */
TEST(cfg_file, parse_device_id3) {
  const char *json = R"json(
{
  "id": [ "0x808j6", "0xbcbd", "0", "0" ]
}
)json";
  json_tokener_error j_err = json_tokener_success;
  json_object *root;
  json_object *j_id;
  opae_pci_device dev;

  root = json_tokener_parse_verbose(json, &j_err);
  ASSERT_NE((void *)NULL, root);

  j_id = parse_json_array(root, "id", NULL);

  EXPECT_NE(0, opae_parse_device_id(j_id, &dev));

  json_object_put(root);
}

/**
 * @test       parse_device_id4
 * @brief      Test: opae_parse_device_id
 * @details    When the j_id parameter points to a JSON array,<br>
 *             and the JSON array has at least 4 entries,<br>
 *             and each of the entries is a string,<br>
 *             but the DID field is not the string representation of an integer,<br>
 *             then the function returns non-zero.<br>
 */
TEST(cfg_file, parse_device_id4) {
  const char *json = R"json(
{
  "id": [ "0x8086", "0xbcjbd", "0", "0" ]
}
)json";
  json_tokener_error j_err = json_tokener_success;
  json_object *root;
  json_object *j_id;
  opae_pci_device dev;

  root = json_tokener_parse_verbose(json, &j_err);
  ASSERT_NE((void *)NULL, root);

  j_id = parse_json_array(root, "id", NULL);

  EXPECT_NE(0, opae_parse_device_id(j_id, &dev));

  json_object_put(root);
}

/**
 * @test       parse_device_id5
 * @brief      Test: opae_parse_device_id
 * @details    When the j_id parameter points to a JSON array,<br>
 *             and the JSON array has at least 4 entries,<br>
 *             and each of the entries is a string,<br>
 *             but the SVID field is not the string representation of an integer,<br>
 *             then the function returns non-zero.<br>
 */
TEST(cfg_file, parse_device_id5) {
  const char *json = R"json(
{
  "id": [ "0x8086", "0xbcbd", "j", "0" ]
}
)json";
  json_tokener_error j_err = json_tokener_success;
  json_object *root;
  json_object *j_id;
  opae_pci_device dev;

  root = json_tokener_parse_verbose(json, &j_err);
  ASSERT_NE((void *)NULL, root);

  j_id = parse_json_array(root, "id", NULL);

  EXPECT_NE(0, opae_parse_device_id(j_id, &dev));

  json_object_put(root);
}

/**
 * @test       parse_device_id6
 * @brief      Test: opae_parse_device_id
 * @details    When the j_id parameter points to a JSON array,<br>
 *             and the JSON array has at least 4 entries,<br>
 *             and each of the entries is a string,<br>
 *             but the SDID field is not the string representation of an integer,<br>
 *             then the function returns non-zero.<br>
 */
TEST(cfg_file, parse_device_id6) {
  const char *json = R"json(
{
  "id": [ "0x8086", "0xbcbd", "0", "j" ]
}
)json";
  json_tokener_error j_err = json_tokener_success;
  json_object *root;
  json_object *j_id;
  opae_pci_device dev;

  root = json_tokener_parse_verbose(json, &j_err);
  ASSERT_NE((void *)NULL, root);

  j_id = parse_json_array(root, "id", NULL);

  EXPECT_NE(0, opae_parse_device_id(j_id, &dev));

  json_object_put(root);
}

/**
 * @test       parse_device_id7
 * @brief      Test: opae_parse_device_id
 * @details    When the j_id parameter points to a JSON array,<br>
 *             and the JSON array has at least 4 entries,<br>
 *             and each of the entries is a string,<br>
 *             and each string is a valid integer,<br>
 *             then the function populates the dev argument and returns zero.<br>
 */
TEST(cfg_file, parse_device_id7) {
  const char *json = R"json(
{
  "id": [ "0x8086", "0xbcbd", "0x3", "0x7" ]
}
)json";
  json_tokener_error j_err = json_tokener_success;
  json_object *root;
  json_object *j_id;
  opae_pci_device dev;

  root = json_tokener_parse_verbose(json, &j_err);
  ASSERT_NE((void *)NULL, root);

  j_id = parse_json_array(root, "id", NULL);

  EXPECT_EQ(0, opae_parse_device_id(j_id, &dev));
  EXPECT_EQ(0x8086, dev.vendor_id);
  EXPECT_EQ(0xbcbd, dev.device_id);
  EXPECT_EQ(0x3, dev.subsystem_vendor_id);
  EXPECT_EQ(0x7, dev.subsystem_device_id);

  json_object_put(root);
}

/**
 * @test       print_libopae_config0
 * @brief      Test: opae_print_libopae_config
 * @details    When the subsystem_vendor_id or subsystem_device_id field<br>
 *             in the given cfg parameter are OPAE_VENDOR_ANY or OPAE_DEVICE_ANY,<br>
 *             then the function prints an aligned '*' character.
 */
TEST(cfg_file, print_libopae_config0) {
  libopae_config_data cfg[] = {
    { 0x8086, 0xbcbd, 0x8086,          0x3210,          "libdummy_plugin.so", "{}", 0 },
    { 0x8086, 0xbcbd, OPAE_VENDOR_ANY, 0x3210,          "libdummy_plugin.so", "{}", 0 },
    { 0x8086, 0xbcbd, 0x8086,          OPAE_DEVICE_ANY, "libdummy_plugin.so", "{}", 0 },
    { 0x8086, 0xbcbd, OPAE_VENDOR_ANY, OPAE_DEVICE_ANY, "libdummy_plugin.so", "{}", 0 },
    {      0,      0,               0,               0, NULL,                 NULL, 0 }
  };
  opae_print_libopae_config(cfg);
}

/**
 * @test       free_libopae_config0
 * @brief      Test: opae_free_libopae_config
 * @details    When the given cfg parameter is not default_libopae_config_table nor NULL,<br>
 *             then the function free's any non-NULL module_library and config_json members,<br>
 *             and it free's the cfg pointer.
 */
TEST(cfg_file, free_libopae_config0) {
  libopae_config_data *cfg = (libopae_config_data *)
    opae_calloc(2, sizeof(libopae_config_data));

  ASSERT_NE((libopae_config_data *)NULL, cfg);

  cfg[0].module_library = opae_strdup("libdummy_plugin.so");
  cfg[0].config_json = opae_strdup("{ \"key\": \"value\" }");

  opae_free_libopae_config(cfg);
}

/**
 * @test       free_libopae_config1
 * @brief      Test: opae_free_libopae_config
 * @details    When the given cfg parameter is default_libopae_config_table or NULL,<br>
 *             then the function returns early.
 */
TEST(cfg_file, free_libopae_config1) {
  opae_free_libopae_config(default_libopae_config_table);
  opae_free_libopae_config(NULL);
}

/**
 * @test       parse_fpgainfo_config0
 * @brief      Test: opae_parse_fpgainfo_config
 * @details    When the given json_input parameter is NULL or if it contains a parse error,<br>
 *             then the function returns default_fpgainfo_config_table.
 */
TEST(cfg_file, parse_fpgainfo_config0) {
  EXPECT_EQ(default_fpgainfo_config_table,
            opae_parse_fpgainfo_config(NULL));

  EXPECT_EQ(default_fpgainfo_config_table,
            opae_parse_fpgainfo_config(opae_strdup("** garbage JSON **")));
}

/**
 * @test       print_fpgainfo_config0
 * @brief      Test: opae_print_fpgainfo_config
 * @details    When the subvendor_id or subdevice_id field<br>
 *             in the given cfg parameter are OPAE_VENDOR_ANY or OPAE_DEVICE_ANY,<br>
 *             then the function prints an aligned '*' character.
 */
TEST(cfg_file, print_fpgainfo_config0) {
  fpgainfo_config_data cfg[] = {
    { 0x8086, 0xbcbd, 0x8086,          0x3210,           0x7, (char *)"libdummy_plugin.so", NULL, "board name" },
    { 0x8086, 0xbcbd, OPAE_VENDOR_ANY, 0x3210,           0x7, (char *)"libdummy_plugin.so", NULL, "board name" },
    { 0x8086, 0xbcbd, 0x8086,          OPAE_DEVICE_ANY, 0x13, (char *)"libdummy_plugin.so", NULL, "board name" },
    { 0x8086, 0xbcbd, OPAE_VENDOR_ANY, OPAE_DEVICE_ANY, 0x13, (char *)"libdummy_plugin.so", NULL, "board name" },
    { 0x8086, 0xbcbd, 0x8086,          0x3210, OPAE_FEATURE_ID_ANY, (char *)"libdummy_plugin.so", NULL, "board name" },
    {      0,      0,               0,               0,    0,                         NULL, NULL, ""           }
  };
  opae_print_fpgainfo_config(cfg);
}

/**
 * @test       free_fpgainfo_config0
 * @brief      Test: opae_free_fpgainfo_config
 * @details    When the given cfg parameter is not default_fpgainfo_config_table nor NULL,<br>
 *             then the function free's any non-NULL board_plugin members,<br>
 *             and it free's the cfg pointer.
 */
TEST(cfg_file, free_fpgainfo_config0) {
  fpgainfo_config_data *cfg = (fpgainfo_config_data *)
    opae_calloc(2, sizeof(fpgainfo_config_data));

  ASSERT_NE((fpgainfo_config_data *)NULL, cfg);

  cfg[0].board_plugin = opae_strdup("libdummy_plugin.so");

  opae_free_fpgainfo_config(cfg);
}

/**
 * @test       free_fpgainfo_config1
 * @brief      Test: opae_free_fpgainfo_config
 * @details    When the given cfg parameter is default_fpgainfo_config_table or NULL,<br>
 *             then the function returns early.
 */
TEST(cfg_file, free_fpgainfo_config1) {
  opae_free_fpgainfo_config(default_fpgainfo_config_table);
  opae_free_fpgainfo_config(NULL);
}

/**
 * @test       parse_fpgad_config0
 * @brief      Test: opae_parse_fpgad_config
 * @details    When the given json_input parameter is NULL or if it contains a parse error,<br>
 *             then the function returns default_fpgad_config_table.
 */
TEST(cfg_file, parse_fpgad_config0) {
  EXPECT_EQ(default_fpgad_config_table,
            opae_parse_fpgad_config(NULL));

  EXPECT_EQ(default_fpgad_config_table,
            opae_parse_fpgad_config(opae_strdup("** garbage JSON **")));
}

/**
 * @test       print_fpgad_config0
 * @brief      Test: opae_print_fpgad_config
 * @details    When the subsystem_vendor_id or subsystem_device_id field<br>
 *             in the given cfg parameter are OPAE_VENDOR_ANY or OPAE_DEVICE_ANY,<br>
 *             then the function prints an aligned '*' character.
 */
TEST(cfg_file, print_fpgad_config0) {
  fpgad_config_data cfg[] = {
    { 0x8086, 0xbcbd, 0x8086,          0x3210,          (char *)"libdummy_plugin.so", 0, NULL, "{}" },
    { 0x8086, 0xbcbd, OPAE_VENDOR_ANY, 0x3210,          (char *)"libdummy_plugin.so", 0, NULL, "{}" },
    { 0x8086, 0xbcbd, 0x8086,          OPAE_DEVICE_ANY, (char *)"libdummy_plugin.so", 0, NULL, "{}" },
    { 0x8086, 0xbcbd, OPAE_VENDOR_ANY, OPAE_DEVICE_ANY, (char *)"libdummy_plugin.so", 0, NULL, "{}" },
    { 0x8086, 0xbcbd, 0x8086,          0x3210,          (char *)"libdummy_plugin.so", 0, NULL, "{}" },
    {      0,      0,      0,               0,          NULL,                         0, NULL, ""   }
  };
  opae_print_fpgad_config(cfg);
}

/**
 * @test       free_fpgad_config0
 * @brief      Test: opae_free_fpgad_config
 * @details    When the given cfg parameter is not default_fpgad_config_table nor NULL,<br>
 *             then the function free's any non-NULL module_library and config_json members,<br>
 *             and it free's the cfg pointer.
 */
TEST(cfg_file, free_fpgad_config0) {
  fpgad_config_data *cfg = (fpgad_config_data *)
    opae_calloc(2, sizeof(fpgad_config_data));

  ASSERT_NE((fpgad_config_data *)NULL, cfg);

  cfg[0].module_library = opae_strdup("libdummy_plugin.so");
  cfg[0].config_json = opae_strdup("{ \"key\": \"value\" }");

  opae_free_fpgad_config(cfg);
}

/**
 * @test       free_fpgad_config1
 * @brief      Test: opae_free_fpgad_config
 * @details    When the given cfg parameter is default_fpgad_config_table or NULL,<br>
 *             then the function returns early.
 */
TEST(cfg_file, free_fpgad_config1) {
  opae_free_fpgad_config(default_fpgad_config_table);
  opae_free_fpgad_config(NULL);
}

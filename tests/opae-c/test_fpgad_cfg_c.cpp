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

#include "mock/opae_fixtures.h"
#include "cfg-file.h"

extern "C" {

typedef struct _fpgad_parse_context {
  fpgad_config_data *cfg;
  fpgad_config_data *current;
  int table_entries;
} fpgad_parse_context;

int resize_fpgad_context(fpgad_parse_context *ctx,
                         int num_new_entries);
int parse_fpgad_plugin_config(json_object *root,
                              const char *cfg_name,
                              fpgad_parse_context *ctx);
fpgad_config_data *
opae_parse_fpgad_json(const char *json_input);

}

using namespace opae::testing;

/**
 * @test       resize0
 * @brief      Test: resize_fpgad_context
 * @details    When ctx->table_entries is 0,<br>
 *             then the function allocates num_new_entries + 1 entries,<br>
 *             and the return value is 0.
 */
TEST(fpgad_cfg, resize0) {
  fpgad_parse_context ctx = { NULL, NULL, 0 };
  const int entries = 2;

  EXPECT_EQ(0, resize_fpgad_context(&ctx, entries));
  EXPECT_NE((fpgad_config_data *)NULL, ctx.cfg);
  EXPECT_EQ(ctx.current, ctx.cfg);
  EXPECT_EQ(entries + 1, ctx.table_entries);

  opae_free(ctx.cfg);
}

class fpgad_cfg_mock_c_p : public opae_base_p<> {};

/**
 * @test       resize_err0
 * @brief      Test: resize_fpgad_context
 * @details    When ctx->table_entries is 0,<br>
 *             and when calloc() fails,<br>
 *             then the function does not modify the context,<br>
 *             and the return value is non-zero.
 */
TEST_P(fpgad_cfg_mock_c_p, resize_err0) {
  system_->invalidate_calloc(0, "resize_fpgad_context");

  fpgad_parse_context ctx = { NULL, NULL, 0 };
  const int entries = 2;

  EXPECT_NE(0, resize_fpgad_context(&ctx, entries));
  EXPECT_EQ((fpgad_config_data *)NULL, ctx.cfg);
  EXPECT_EQ(0, ctx.table_entries);
}

/**
 * @test       resize_err1
 * @brief      Test: resize_fpgad_context
 * @details    When ctx->table_entries is non-zero,<br>
 *             and when calloc() fails,<br>
 *             then the function does not modify the context,<br>
 *             and the return value is non-zero.
 */
TEST_P(fpgad_cfg_mock_c_p, resize_err1) {
  system_->invalidate_calloc(0, "resize_fpgad_context");

  const int entries = 2;
  fpgad_config_data *cd = (fpgad_config_data *)
    opae_calloc(entries + 1, sizeof(fpgad_config_data));
  ASSERT_NE((fpgad_config_data *)NULL, cd);

  fpgad_parse_context ctx = { cd, &cd[1], entries };

  EXPECT_NE(0, resize_fpgad_context(&ctx, entries));
  EXPECT_EQ(cd, ctx.cfg);
  EXPECT_EQ(&cd[1], ctx.current);
  EXPECT_EQ(entries, ctx.table_entries);

  opae_free(cd);
}

/**
 * @test       parse_config_err3
 * @brief      Test: parse_fpgad_plugin_config
 * @details    When the input JSON string has a "configurations" object,<br>
 *             and that object has a section matching the cfg_name parameter,<br>
 *             and that object has an "enabled" key set to true,<br>
 *             and a non-empty "devices" key is given,<br>
 *             but calloc fails to allocate pci_devices,<br>
 *             the function returns non-zero.
 */
TEST_P(fpgad_cfg_mock_c_p, parse_config_err3) {
  const char *json = R"json(
{
  "configurations": {

    "ofs": {
      "enabled": true,
      "devices": [
        { "name": "ofs1_pf", "id": [ "0x8086", "0xbcce", "0x8086", "0" ] }
      ]
    }

  }
}
)json";
  json_tokener_error j_err = json_tokener_success;
  json_object *root;

  root = json_tokener_parse_verbose(json, &j_err);
  ASSERT_NE((void *)NULL, root);

  fpgad_parse_context ctx = { NULL, NULL, 0 };
  system_->invalidate_calloc(0, "parse_fpgad_plugin_config");
  EXPECT_NE(0, parse_fpgad_plugin_config(root, "ofs", &ctx));

  json_object_put(root);
}

/**
 * @test       parse_config_err8
 * @brief      Test: parse_fpgad_plugin_config
 * @details    When the input JSON string has a "configurations" object,<br>
 *             and that object has a section matching the cfg_name parameter,<br>
 *             and that object has an "enabled" key set to true,<br>
 *             and the "devices" key is present and non-empty,<br>
 *             and each "devices" entry has a "name" field,<br>
 *             and each "devices" entry has an "id" field,<br>
 *             and each "id" field is valid,<br>
 *             and there is an "opae" object,<br>
 *             and the "fpgad" array in the object has items,<br>
 *             and the items have an enabled flag set to true,<br>
 *             and the item's "devices" array is present,<br>
 *             but the calloc() call in resize_fpgad_context() fails,<br>
 *             the function returns non-zero.
 */
TEST_P(fpgad_cfg_mock_c_p, parse_config_err8) {
  const char *json = R"json(
{
  "configurations": {

    "ofs": {
      "enabled": true,
      "devices": [
        { "name": "ofs0_pf", "id": [ "0x8086", "0xbcce", "*", "*" ] }
      ],

      "opae": {
        "fpgad": [
          {
            "enabled": true,
            "devices": []
          }
        ]
      }
    }

  }
}
)json";
  json_tokener_error j_err = json_tokener_success;
  json_object *root;

  root = json_tokener_parse_verbose(json, &j_err);
  ASSERT_NE((void *)NULL, root);

  system_->invalidate_calloc(0, "resize_fpgad_context");

  fpgad_parse_context ctx = { NULL, NULL, 0 };
  EXPECT_NE(0, parse_fpgad_plugin_config(root, "ofs", &ctx));

  EXPECT_EQ((fpgad_config_data *)NULL, ctx.cfg);
  EXPECT_EQ(0, ctx.table_entries);

  json_object_put(root);
}

/**
 * @test       parse_config_err13
 * @brief      Test: parse_plugin_config
 * @details    When the input JSON string has a "configurations" object,<br>
 *             and that object has a section matching the cfg_name parameter,<br>
 *             and that object has an "enabled" key set to true,<br>
 *             and the "devices" key is present and non-empty,<br>
 *             and each "devices" entry has a "name" field,<br>
 *             and each "devices" entry has an "id" field,<br>
 *             and each "id" field is valid,<br>
 *             and there is an "opae" object,<br>
 *             and the "fpgad" array in the object has items,<br>
 *             and the items have an enabled flag set to true,<br>
 *             and the item's "devices" array is non-empty,<br>
 *             and the item's "module" key is non-empty,<br>
 *             and the item's "configuration" key is present,<br>
 *             and each devices[j] entry is found in the config's "devices" array,<br>
 *             but strdup() for module_library or config_json fails,<br>
 *             the function returns non-zero.
 */
TEST_P(fpgad_cfg_mock_c_p, parse_config_err13) {
  const char *json = R"json(
{
  "configurations": {

    "ofs": {
      "enabled": true,
      "devices": [
        { "name": "ofs0_pf", "id": [ "0x8086", "0xbcce", "*", "*" ] }
      ],

      "opae": {
        "fpgad": [
          {
            "enabled": true,
            "module": "libfpgad-vc.so",
            "devices": [ "ofs0_pf" ],
            "configuration": {}
          }
        ]
      }
    }

  }
}
)json";
  json_tokener_error j_err = json_tokener_success;
  json_object *root;

  root = json_tokener_parse_verbose(json, &j_err);
  ASSERT_NE((void *)NULL, root);

  system_->invalidate_strdup(1, "parse_fpgad_plugin_config");

  fpgad_parse_context ctx = { NULL, NULL, 0 };
  EXPECT_NE(0, parse_fpgad_plugin_config(root, "ofs", &ctx));

  EXPECT_NE((fpgad_config_data *)NULL, ctx.cfg);
  EXPECT_EQ(2, ctx.table_entries);

  EXPECT_STREQ("libfpgad-vc.so", ctx.cfg[0].module_library);
  EXPECT_EQ(NULL, ctx.cfg[0].config_json);

  EXPECT_EQ(NULL, ctx.cfg[1].module_library);
  EXPECT_EQ(NULL, ctx.cfg[1].config_json);

  opae_free_fpgad_config(ctx.cfg);

  json_object_put(root);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(fpgad_cfg_mock_c_p);
INSTANTIATE_TEST_SUITE_P(fpgad_cfg_c, fpgad_cfg_mock_c_p,
                         ::testing::ValuesIn(test_platform::mock_platforms({
                                                                             "dfl-n6000-sku0",
                                                                             "dfl-n6000-sku1",
                                                                             "dfl-c6100"
                                                                           })));

/**
 * @test       resize1
 * @brief      Test: resize_fpgad_context
 * @details    When ctx->table_entries is non-zero,<br>
 *             then the function allocates a new config buffer,<br>
 *             copying all the orignal entries and preserving the<br>
 *             relative location of the current pointer.
 */
TEST(fpgad_cfg, resize1) {
  const int entries = 2;

  fpgad_config_data *cd = (fpgad_config_data *)
    opae_calloc(entries + 1, sizeof(fpgad_config_data));
  ASSERT_NE((fpgad_config_data *)NULL, cd);

  char *module_library = opae_strdup("libdummy_plugin.so");
  char *config_json = opae_strdup("{}");

  cd[0].module_library = module_library;
  cd[0].config_json = config_json;

  fpgad_parse_context ctx = { cd, &cd[1], entries };

  // Add one more config entry.
  EXPECT_EQ(0, resize_fpgad_context(&ctx, 1));

  EXPECT_NE(cd, ctx.cfg);
  EXPECT_NE(&cd[1], ctx.current);

  EXPECT_EQ(entries + 1, ctx.table_entries);
  ASSERT_NE((fpgad_config_data *)NULL, ctx.cfg);
  EXPECT_EQ(module_library, ctx.cfg[0].module_library);
  EXPECT_EQ(config_json, ctx.cfg[0].config_json);

  EXPECT_EQ(ctx.current, &ctx.cfg[1]);

  EXPECT_EQ((char *)NULL, ctx.cfg[1].module_library);
  EXPECT_EQ((char *)NULL, ctx.cfg[1].config_json);
  EXPECT_EQ((char *)NULL, ctx.cfg[2].module_library);
  EXPECT_EQ((char *)NULL, ctx.cfg[2].config_json);

  opae_free(ctx.cfg);
  opae_free(module_library);
  opae_free(config_json);
}

/**
 * @test       parse_config_err0
 * @brief      Test: parse_fpgad_plugin_config
 * @details    When the input JSON string has no "configurations" object,<br>
 *             the function returns non-zero.
 */
TEST(fpgad_cfg, parse_config_err0) {
  const char *json = R"json(
{
}
)json";
  json_tokener_error j_err = json_tokener_success;
  json_object *root;

  root = json_tokener_parse_verbose(json, &j_err);
  ASSERT_NE((void *)NULL, root);

  fpgad_parse_context ctx = { NULL, NULL, 0 };
  EXPECT_NE(0, parse_fpgad_plugin_config(root, "", &ctx));

  json_object_put(root);
}

/**
 * @test       parse_config_err1
 * @brief      Test: parse_fpgad_plugin_config
 * @details    When the input JSON string has a "configurations" object,<br>
 *             but that object has no section matching the cfg_name parameter,<br>
 *             the function returns non-zero.
 */
TEST(fpgad_cfg, parse_config_err1) {
  const char *json = R"json(
{
  "configurations": {
  }
}
)json";
  json_tokener_error j_err = json_tokener_success;
  json_object *root;

  root = json_tokener_parse_verbose(json, &j_err);
  ASSERT_NE((void *)NULL, root);

  fpgad_parse_context ctx = { NULL, NULL, 0 };
  EXPECT_NE(0, parse_fpgad_plugin_config(root, "ofs", &ctx));

  json_object_put(root);
}

/**
 * @test       parse_config_enabled0
 * @brief      Test: parse_fpgad_plugin_config
 * @details    When the input JSON string has a "configurations" object,<br>
 *             and that object has a section matching the cfg_name parameter,<br>
 *             but that section has no "enabled" key,<br>
 *             the function returns early with zero.
 */
TEST(fpgad_cfg, parse_config_enabled0) {
  const char *json = R"json(
{
  "configurations": {

    "ofs": {

    }

  }
}
)json";
  json_tokener_error j_err = json_tokener_success;
  json_object *root;

  root = json_tokener_parse_verbose(json, &j_err);
  ASSERT_NE((void *)NULL, root);

  fpgad_parse_context ctx = { NULL, NULL, 0 };
  EXPECT_EQ(0, parse_fpgad_plugin_config(root, "ofs", &ctx));

  json_object_put(root);
}

/**
 * @test       parse_config_err2
 * @brief      Test: parse_fpgad_plugin_config
 * @details    When the input JSON string has a "configurations" object,<br>
 *             and that object has a section matching the cfg_name parameter,<br>
 *             and that object has an "enabled" key set to true,<br>
 *             but the "devices" key is missing or empty,<br>
 *             the function returns non-zero.
 */
TEST(fpgad_cfg, parse_config_err2) {
  const char *json = R"json(
{
  "configurations": {

    "ofs": {
      "enabled": true,
      "devices": []
    }

  }
}
)json";
  json_tokener_error j_err = json_tokener_success;
  json_object *root;

  root = json_tokener_parse_verbose(json, &j_err);
  ASSERT_NE((void *)NULL, root);

  fpgad_parse_context ctx = { NULL, NULL, 0 };
  EXPECT_NE(0, parse_fpgad_plugin_config(root, "ofs", &ctx));

  json_object_put(root);
}

/**
 * @test       parse_config_err4
 * @brief      Test: parse_fpgad_plugin_config
 * @details    When the input JSON string has a "configurations" object,<br>
 *             and that object has a section matching the cfg_name parameter,<br>
 *             and that object has an "enabled" key set to true,<br>
 *             and the "devices" key is present and non-empty,<br>
 *             but some "devices" entry has no "name" field,<br>
 *             the function returns non-zero.
 */
TEST(fpgad_cfg, parse_config_err4) {
  const char *json = R"json(
{
  "configurations": {

    "ofs": {
      "enabled": true,
      "devices": [
        { "blame": "ofs0_pf" }
      ]
    }

  }
}
)json";
  json_tokener_error j_err = json_tokener_success;
  json_object *root;

  root = json_tokener_parse_verbose(json, &j_err);
  ASSERT_NE((void *)NULL, root);

  fpgad_parse_context ctx = { NULL, NULL, 0 };
  EXPECT_NE(0, parse_fpgad_plugin_config(root, "ofs", &ctx));

  json_object_put(root);
}

/**
 * @test       parse_config_err5
 * @brief      Test: parse_fpgad_plugin_config
 * @details    When the input JSON string has a "configurations" object,<br>
 *             and that object has a section matching the cfg_name parameter,<br>
 *             and that object has an "enabled" key set to true,<br>
 *             and the "devices" key is present and non-empty,<br>
 *             and each "devices" entry has a "name" field,<br>
 *             but some "devices" entry has no "id" field,<br>
 *             the function returns non-zero.
 */
TEST(fpgad_cfg, parse_config_err5) {
  const char *json = R"json(
{
  "configurations": {

    "ofs": {
      "enabled": true,
      "devices": [
        { "name": "ofs0_pf" }
      ]
    }

  }
}
)json";
  json_tokener_error j_err = json_tokener_success;
  json_object *root;

  root = json_tokener_parse_verbose(json, &j_err);
  ASSERT_NE((void *)NULL, root);

  fpgad_parse_context ctx = { NULL, NULL, 0 };
  EXPECT_NE(0, parse_fpgad_plugin_config(root, "ofs", &ctx));

  json_object_put(root);
}

/**
 * @test       parse_config_err6
 * @brief      Test: parse_fpgad_plugin_config
 * @details    When the input JSON string has a "configurations" object,<br>
 *             and that object has a section matching the cfg_name parameter,<br>
 *             and that object has an "enabled" key set to true,<br>
 *             and the "devices" key is present and non-empty,<br>
 *             and each "devices" entry has a "name" field,<br>
 *             and each "devices" entry has an "id" field,<br>
 *             but some "id" field is invalid,<br>
 *             the function returns non-zero.
 */
TEST(fpgad_cfg, parse_config_err6) {
  const char *json = R"json(
{
  "configurations": {

    "ofs": {
      "enabled": true,
      "devices": [
        { "name": "ofs0_pf", "id": [ "garbage", "junk", "trash", "litter" ] }
      ]
    }

  }
}
)json";
  json_tokener_error j_err = json_tokener_success;
  json_object *root;

  root = json_tokener_parse_verbose(json, &j_err);
  ASSERT_NE((void *)NULL, root);

  fpgad_parse_context ctx = { NULL, NULL, 0 };
  EXPECT_NE(0, parse_fpgad_plugin_config(root, "ofs", &ctx));

  json_object_put(root);
}

/**
 * @test       parse_config_err7
 * @brief      Test: parse_fpgad_plugin_config
 * @details    When the input JSON string has a "configurations" object,<br>
 *             and that object has a section matching the cfg_name parameter,<br>
 *             and that object has an "enabled" key set to true,<br>
 *             and the "devices" key is present and non-empty,<br>
 *             and each "devices" entry has a "name" field,<br>
 *             and each "devices" entry has an "id" field,<br>
 *             and each "id" field is valid,<br>
 *             but there is no "opae" key,<br>
 *             the function returns non-zero.
 */
TEST(fpgad_cfg, parse_config_err7) {
  const char *json = R"json(
{
  "configurations": {

    "ofs": {
      "enabled": true,
      "devices": [
        { "name": "ofs0_pf", "id": [ "0x8086", "0xbcce", "*", "*" ] }
      ]
    }

  }
}
)json";
  json_tokener_error j_err = json_tokener_success;
  json_object *root;

  root = json_tokener_parse_verbose(json, &j_err);
  ASSERT_NE((void *)NULL, root);

  fpgad_parse_context ctx = { NULL, NULL, 0 };
  EXPECT_NE(0, parse_fpgad_plugin_config(root, "ofs", &ctx));

  json_object_put(root);
}

/**
 * @test       parse_config_0
 * @brief      Test: parse_plugin_config
 * @details    When the input JSON string has a "configurations" object,<br>
 *             and that object has a section matching the cfg_name parameter,<br>
 *             and that object has an "enabled" key set to true,<br>
 *             and the "devices" key is present and non-empty,<br>
 *             and each "devices" entry has a "name" field,<br>
 *             and each "devices" entry has an "id" field,<br>
 *             and each "id" field is valid,<br>
 *             and there is an "opae" object,<br>
 *             but the "fpgad" array in the object is empty,<br>
 *             the function returns zero.
 */
TEST(fpgad_cfg, parse_config_0) {
  const char *json = R"json(
{
  "configurations": {

    "ofs": {
      "enabled": true,
      "devices": [
        { "name": "ofs0_pf", "id": [ "0x8086", "0xbcce", "*", "*" ] }
      ],

      "opae": {
        "fpgad": []
      }
    }

  }
}
)json";
  json_tokener_error j_err = json_tokener_success;
  json_object *root;

  root = json_tokener_parse_verbose(json, &j_err);
  ASSERT_NE((void *)NULL, root);

  fpgad_parse_context ctx = { NULL, NULL, 0 };
  EXPECT_EQ(0, parse_fpgad_plugin_config(root, "ofs", &ctx));

  json_object_put(root);
}

/**
 * @test       parse_config_1
 * @brief      Test: parse_plugin_config
 * @details    When the input JSON string has a "configurations" object,<br>
 *             and that object has a section matching the cfg_name parameter,<br>
 *             and that object has an "enabled" key set to true,<br>
 *             and the "devices" key is present and non-empty,<br>
 *             and each "devices" entry has a "name" field,<br>
 *             and each "devices" entry has an "id" field,<br>
 *             and each "id" field is valid,<br>
 *             and there is an "opae" object,<br>
 *             and the "fpgad" array in the object has items,<br>
 *             but the items have an enabled flag set to false,<br>
 *             the function returns zero.
 */
TEST(fpgad_cfg, parse_config_1) {
  const char *json = R"json(
{
  "configurations": {

    "ofs": {
      "enabled": true,
      "devices": [
        { "name": "ofs0_pf", "id": [ "0x8086", "0xbcce", "*", "*" ] }
      ],

      "opae": {
        "fpgad": [
          {
            "enabled": false
          }
        ]
      }
    }

  }
}
)json";
  json_tokener_error j_err = json_tokener_success;
  json_object *root;

  root = json_tokener_parse_verbose(json, &j_err);
  ASSERT_NE((void *)NULL, root);

  fpgad_parse_context ctx = { NULL, NULL, 0 };
  EXPECT_EQ(0, parse_fpgad_plugin_config(root, "ofs", &ctx));

  EXPECT_NE((fpgad_config_data *)NULL, ctx.cfg);
  EXPECT_EQ(1, ctx.table_entries);
  EXPECT_EQ(NULL, ctx.cfg->module_library);

  opae_free_fpgad_config(ctx.cfg);

  json_object_put(root);
}

/**
 * @test       parse_config_2
 * @brief      Test: parse_fpgad_plugin_config
 * @details    When the input JSON string has a "configurations" object,<br>
 *             and that object has a section matching the cfg_name parameter,<br>
 *             and that object has an "enabled" key set to true,<br>
 *             and the "devices" key is present and non-empty,<br>
 *             and each "devices" entry has a "name" field,<br>
 *             and each "devices" entry has an "id" field,<br>
 *             and each "id" field is valid,<br>
 *             and there is an "opae" object,<br>
 *             and the "fpgad" array in the object has items,<br>
 *             and the items have an enabled flag set to true,<br>
 *             but the item's "devices" array is empty,<br>
 *             the function returns zero.
 */
TEST(fpgad_cfg, parse_config_2) {
  const char *json = R"json(
{
  "configurations": {

    "ofs": {
      "enabled": true,
      "devices": [
        { "name": "ofs0_pf", "id": [ "0x8086", "0xbcce", "*", "*" ] }
      ],

      "opae": {
        "fpgad": [
          {
            "enabled": true,
            "devices": []
          }
        ]
      }
    }

  }
}
)json";
  json_tokener_error j_err = json_tokener_success;
  json_object *root;

  root = json_tokener_parse_verbose(json, &j_err);
  ASSERT_NE((void *)NULL, root);

  fpgad_parse_context ctx = { NULL, NULL, 0 };
  EXPECT_EQ(0, parse_fpgad_plugin_config(root, "ofs", &ctx));

  EXPECT_NE((fpgad_config_data *)NULL, ctx.cfg);
  EXPECT_EQ(1, ctx.table_entries);
  EXPECT_EQ(NULL, ctx.cfg->module_library);

  opae_free_fpgad_config(ctx.cfg);

  json_object_put(root);
}

/**
 * @test       parse_config_err9
 * @brief      Test: parse_plugin_config
 * @details    When the input JSON string has a "configurations" object,<br>
 *             and that object has a section matching the cfg_name parameter,<br>
 *             and that object has an "enabled" key set to true,<br>
 *             and the "devices" key is present and non-empty,<br>
 *             and each "devices" entry has a "name" field,<br>
 *             and each "devices" entry has an "id" field,<br>
 *             and each "id" field is valid,<br>
 *             and there is an "opae" object,<br>
 *             and the "fpgad" array in the object has items,<br>
 *             and the items have an enabled flag set to true,<br>
 *             and the item's "devices" array is non-empty,<br>
 *             but the item's "module" key is empty,<br>
 *             the function returns non-zero.
 */
TEST(fpgad_cfg, parse_config_err9) {
  const char *json = R"json(
{
  "configurations": {

    "ofs": {
      "enabled": true,
      "devices": [
        { "name": "ofs0_pf", "id": [ "0x8086", "0xbcce", "*", "*" ] }
      ],

      "opae": {
        "fpgad": [
          {
            "enabled": true,
            "module": "",
            "devices": [ "ofs0_pf" ]
          }
        ]
      }
    }

  }
}
)json";
  json_tokener_error j_err = json_tokener_success;
  json_object *root;

  root = json_tokener_parse_verbose(json, &j_err);
  ASSERT_NE((void *)NULL, root);

  fpgad_parse_context ctx = { NULL, NULL, 0 };
  EXPECT_NE(0, parse_fpgad_plugin_config(root, "ofs", &ctx));

  EXPECT_NE((fpgad_config_data *)NULL, ctx.cfg);
  EXPECT_EQ(2, ctx.table_entries);
  EXPECT_EQ(NULL, ctx.cfg->module_library);

  opae_free_fpgad_config(ctx.cfg);

  json_object_put(root);
}

/**
 * @test       parse_config_err10
 * @brief      Test: parse_fpgad_plugin_config
 * @details    When the input JSON string has a "configurations" object,<br>
 *             and that object has a section matching the cfg_name parameter,<br>
 *             and that object has an "enabled" key set to true,<br>
 *             and the "devices" key is present and non-empty,<br>
 *             and each "devices" entry has a "name" field,<br>
 *             and each "devices" entry has an "id" field,<br>
 *             and each "id" field is valid,<br>
 *             and there is an "opae" object,<br>
 *             and the "fpgad" array in the object has items,<br>
 *             and the items have an enabled flag set to true,<br>
 *             and the item's "devices" array is non-empty,<br>
 *             and the item's "module" key is non-empty,<br>
 *             but the item's "configuration" key is missing,<br>
 *             the function returns non-zero.
 */
TEST(fpgad_cfg, parse_config_err10) {
  const char *json = R"json(
{
  "configurations": {

    "ofs": {
      "enabled": true,
      "devices": [
        { "name": "ofs0_pf", "id": [ "0x8086", "0xbcce", "*", "*" ] }
      ],

      "opae": {
        "fpgad": [
          {
            "enabled": true,
            "module": "libfpgad-vc.so",
            "devices": [ "ofs0_pf" ]
          }
        ]
      }
    }

  }
}
)json";
  json_tokener_error j_err = json_tokener_success;
  json_object *root;

  root = json_tokener_parse_verbose(json, &j_err);
  ASSERT_NE((void *)NULL, root);

  fpgad_parse_context ctx = { NULL, NULL, 0 };
  EXPECT_NE(0, parse_fpgad_plugin_config(root, "ofs", &ctx));

  EXPECT_NE((fpgad_config_data *)NULL, ctx.cfg);
  EXPECT_EQ(2, ctx.table_entries);
  EXPECT_EQ(NULL, ctx.cfg->module_library);

  opae_free_fpgad_config(ctx.cfg);

  json_object_put(root);
}

/**
 * @test       parse_config_err11
 * @brief      Test: parse_fpgad_plugin_config
 * @details    When the input JSON string has a "configurations" object,<br>
 *             and that object has a section matching the cfg_name parameter,<br>
 *             and that object has an "enabled" key set to true,<br>
 *             and the "devices" key is present and non-empty,<br>
 *             and each "devices" entry has a "name" field,<br>
 *             and each "devices" entry has an "id" field,<br>
 *             and each "id" field is valid,<br>
 *             and there is an "opae" object,<br>
 *             and the "fpgad" array in the object has items,<br>
 *             and the items have an enabled flag set to true,<br>
 *             and the item's "devices" array is non-empty,<br>
 *             and the item's "module" key is non-empty,<br>
 *             and the item's "configuration" key is present,<br>
 *             but some devices[j] entry is not a string,<br>
 *             the function returns non-zero.
 */
TEST(fpgad_cfg, parse_config_err11) {
  const char *json = R"json(
{
  "configurations": {

    "ofs": {
      "enabled": true,
      "devices": [
        { "name": "ofs0_pf", "id": [ "0x8086", "0xbcce", "*", "*" ] }
      ],

      "opae": {
        "fpgad": [
          {
            "enabled": true,
            "module": "libfpgad-vc.so",
            "devices": [ "ofs0_pf", 3 ],
            "configuration": {}
          }
        ]
      }
    }

  }
}
)json";
  json_tokener_error j_err = json_tokener_success;
  json_object *root;

  root = json_tokener_parse_verbose(json, &j_err);
  ASSERT_NE((void *)NULL, root);

  fpgad_parse_context ctx = { NULL, NULL, 0 };
  EXPECT_NE(0, parse_fpgad_plugin_config(root, "ofs", &ctx));

  EXPECT_NE((fpgad_config_data *)NULL, ctx.cfg);
  EXPECT_EQ(3, ctx.table_entries);

  EXPECT_STREQ("libfpgad-vc.so", ctx.cfg[0].module_library);
  EXPECT_STREQ("{}", ctx.cfg[0].config_json);

  EXPECT_EQ(NULL, ctx.cfg[1].module_library);
  EXPECT_EQ(NULL, ctx.cfg[1].config_json);

  opae_free_fpgad_config(ctx.cfg);

  json_object_put(root);
}

/**
 * @test       parse_config_err12
 * @brief      Test: parse_fpgad_plugin_config
 * @details    When the input JSON string has a "configurations" object,<br>
 *             and that object has a section matching the cfg_name parameter,<br>
 *             and that object has an "enabled" key set to true,<br>
 *             and the "devices" key is present and non-empty,<br>
 *             and each "devices" entry has a "name" field,<br>
 *             and each "devices" entry has an "id" field,<br>
 *             and each "id" field is valid,<br>
 *             and there is an "opae" object,<br>
 *             and the "fpgad" array in the object has items,<br>
 *             and the items have an enabled flag set to true,<br>
 *             and the item's "devices" array is non-empty,<br>
 *             and the item's "module" key is non-empty,<br>
 *             and the item's "configuration" key is present,<br>
 *             but some devices[j] entry is not found in the config's "devices" array,<br>
 *             the function returns non-zero.
 */
TEST(fpgad_cfg, parse_config_err12) {
  const char *json = R"json(
{
  "configurations": {

    "ofs": {
      "enabled": true,
      "devices": [
        { "name": "ofs0_pf", "id": [ "0x8086", "0xbcce", "*", "*" ] }
      ],

      "opae": {
        "fpgad": [
          {
            "enabled": true,
            "module": "libfpgad-vc.so",
            "devices": [ "ofs0_pf", "garbage" ],
            "configuration": {}
          }
        ]
      }
    }

  }
}
)json";
  json_tokener_error j_err = json_tokener_success;
  json_object *root;

  root = json_tokener_parse_verbose(json, &j_err);
  ASSERT_NE((void *)NULL, root);

  fpgad_parse_context ctx = { NULL, NULL, 0 };
  EXPECT_NE(0, parse_fpgad_plugin_config(root, "ofs", &ctx));

  EXPECT_NE((fpgad_config_data *)NULL, ctx.cfg);
  EXPECT_EQ(3, ctx.table_entries);

  EXPECT_STREQ("libfpgad-vc.so", ctx.cfg[0].module_library);
  EXPECT_STREQ("{}", ctx.cfg[0].config_json);

  EXPECT_EQ(NULL, ctx.cfg[1].module_library);
  EXPECT_EQ(NULL, ctx.cfg[1].config_json);

  opae_free_fpgad_config(ctx.cfg);

  json_object_put(root);
}

/**
 * @test       parse_fpgad_err0
 * @brief      Test: opae_parse_fpgad_json
 * @details    When the json_input parameter is not valid JSON,<br>
 *             then the function returns NULL.<br>
 */
TEST(fpgad_cfg, parse_fpgad_err0) {
  char *json = opae_strdup("This is not JSON");
  EXPECT_EQ((fpgad_config_data *)NULL, opae_parse_fpgad_json(json));
}

/**
 * @test       parse_fpgad_err1
 * @brief      Test: opae_parse_fpgad_json
 * @details    When the json_input parameter is valid JSON,<br>
 *             and the JSON includes a valid "configs" array,<br>
 *             but some config is in error,<br>
 *             then the function returns NULL.<br>
 */
TEST(fpgad_cfg, parse_fpgad_err1) {
  const char *json = R"json(
{
  "configurations": {

    "ofs": {
      "enabled": true,
      "devices": [
        { "name": "ofs0_pf", "id": [ "0x8086", "0xbcce", "*", "*" ] }
      ],

      "opae": {
        "fpgad": [
          {
            "enabled": true,
            "module": "",
            "devices": [ "ofs0_pf" ]
          }
        ]
      }
    }

  },

  "configs": [
    "ofs"
  ]
}
)json";

  char *json_dup = opae_strdup(json);
  EXPECT_EQ((fpgad_config_data *)NULL, opae_parse_fpgad_json(json_dup));
}

/**
 * @test       parse_fpgad_err2
 * @brief      Test: opae_parse_fpgad_json
 * @details    When the json_input parameter is valid JSON,<br>
 *             but the JSON includes an invalid "configs" array,<br>
 *             then the function returns NULL.<br>
 */
TEST(fpgad_cfg, parse_fpgad_err2) {
  const char *json = R"json(
{
  "configurations": {

    "ofs": {
      "enabled": true,
      "devices": [
        { "name": "ofs0_pf", "id": [ "0x8086", "0xbcce", "*", "*" ] }
      ],

      "opae": {
        "fpgad": [
          {
            "enabled": true,
            "module": "libfpgad-vc.so",
            "devices": [ "ofs0_pf" ]
          }
        ]
      }
    }

  },

  "configs": 3
}
)json";

  char *json_dup = opae_strdup(json);
  EXPECT_EQ((fpgad_config_data *)NULL, opae_parse_fpgad_json(json_dup));
}

/**
 * @test       parse_fpgad_0
 * @brief      Test: opae_parse_fpgad_json
 * @details    When the json_input parameter is valid,<br>
 *             then the function returns the appropriate table.<br>
 */
TEST(fpgad_cfg, parse_fpgad_0) {
  const char *json = R"json(
{
  "configurations": {

    "ofs": {
      "enabled": true,
      "devices": [
        { "name": "ofs0_pf", "id": [ "0x8086", "0xaf00", "0x8086", "0" ] },
        { "name": "ofs0_vf", "id": [ "0x8086", "0xaf01", "0x8086", "0" ] },
        { "name": "ofs1_pf", "id": [ "0x8086", "0xbcce", "0x8086", "0" ] },
        { "name": "ofs1_vf", "id": [ "0x8086", "0xbccf", "0x8086", "0" ] }
      ],

      "opae": {
        "fpgad": [
          {
            "enabled": true,
            "module": "libfpgad-vc.so",
            "devices": [ "ofs0_pf", "ofs1_pf" ],
            "configuration": {}
          }
        ]
      }
    }

  },

  "configs": [
    "ofs"
  ]
}
)json";

  char *json_dup = opae_strdup(json);
  fpgad_config_data *cfg, *c;

  cfg = c = opae_parse_fpgad_json(json_dup);
  ASSERT_NE((fpgad_config_data *)NULL, cfg);

  // ofs0_pf / libfpgad-vc.so
  EXPECT_EQ(0x8086, c->vendor_id);
  EXPECT_EQ(0xaf00, c->device_id);
  EXPECT_EQ(0x8086, c->subsystem_vendor_id);
  EXPECT_EQ(0, c->subsystem_device_id);
  EXPECT_STREQ("libfpgad-vc.so", c->module_library);
  EXPECT_EQ(0, c->flags);
  EXPECT_EQ(NULL, c->dl_handle);
  EXPECT_STREQ("{}", c->config_json);

  // ofs1_pf / libfpgad-vc.so
  ++c;
  EXPECT_EQ(0x8086, c->vendor_id);
  EXPECT_EQ(0xbcce, c->device_id);
  EXPECT_EQ(0x8086, c->subsystem_vendor_id);
  EXPECT_EQ(0, c->subsystem_device_id);
  EXPECT_STREQ("libfpgad-vc.so", c->module_library);
  EXPECT_EQ(0, c->flags);
  EXPECT_EQ(NULL, c->dl_handle);
  EXPECT_STREQ("{}", c->config_json);

  // null terminator
  ++c;
  EXPECT_EQ(0, c->vendor_id);
  EXPECT_EQ(0, c->device_id);
  EXPECT_EQ(0, c->subsystem_vendor_id);
  EXPECT_EQ(0, c->subsystem_device_id);
  EXPECT_EQ(NULL, c->module_library);
  EXPECT_EQ(0, c->flags);
  EXPECT_EQ(NULL, c->dl_handle);
  EXPECT_EQ(NULL, c->config_json);

  opae_free_fpgad_config(cfg);
}

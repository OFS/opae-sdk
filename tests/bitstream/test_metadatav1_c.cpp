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

#include "libbitstream/bitstream.h"
#include "libbitstream/metadatav1.h"

extern "C" {

fpga_result opae_bitstream_parse_accelerator_cluster_v1(json_object *j_cluster,
				opae_metadata_accelerator_cluster_v1 *cluster);

fpga_result opae_bitstream_parse_afu_image_v1(json_object *j_afu_image,
					      opae_metadata_afu_image_v1 *img,
					      fpga_guid pr_interface_id);

}

#include <config.h>
#include <opae/fpga.h>

#include "gtest/gtest.h"
#include "mock/test_system.h"
#include "safe_string/safe_string.h"

using namespace opae::testing;

class metadatav1_c_p : public ::testing::TestWithParam<std::string> {
 protected:

  virtual void SetUp() override {
    std::string platform_key = GetParam();
    ASSERT_TRUE(test_platform::exists(platform_key));
    platform_ = test_platform::get(platform_key);
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);

    j_root_ = nullptr;
  }

  virtual void TearDown() override {

    if (j_root_)
      json_object_put(j_root_);

    system_->finalize();
  }

  json_object *parse(const char *json_str)
  {
    enum json_tokener_error j_err = json_tokener_success;
    return j_root_ = json_tokener_parse_verbose(json_str, &j_err);
  }

  json_object *j_root_;
  test_platform platform_;
  test_system *system_;
};

/**
 * @test       cluster_err0
 * @brief      Test: opae_bitstream_parse_accelerator_cluster_v1
 * @details    If the given json_object contains no,<br>
 *             "total-contexts" key,<br>
 *             the fn returns FPGA_EXCEPTION.<br>
 */
TEST_P(metadatav1_c_p, cluster_err0) {
    const char *mdata =
    R"mdata({
"name": "nlb_400",
"accelerator-type-uuid": "d8424dc4-a4a3-c413-f89e-433683f9040b"
})mdata";
  json_object *j_cluster;

  j_cluster = parse(mdata);
  ASSERT_NE(j_cluster, nullptr);

  opae_metadata_accelerator_cluster_v1 cluster;
  memset(&cluster, 0, sizeof(cluster));

  EXPECT_EQ(opae_bitstream_parse_accelerator_cluster_v1(j_cluster,
							&cluster),
            FPGA_EXCEPTION);

  EXPECT_EQ(cluster.total_contexts, 0);
  EXPECT_EQ(cluster.name, nullptr);
  EXPECT_EQ(cluster.accelerator_type_uuid, nullptr);
}

/**
 * @test       cluster_err1
 * @brief      Test: opae_bitstream_parse_accelerator_cluster_v1
 * @details    If the given json_object contains no,<br>
 *             "name" key,<br>
 *             the fn returns FPGA_EXCEPTION.<br>
 */
TEST_P(metadatav1_c_p, cluster_err1) {
    const char *mdata =
    R"mdata({
"total-contexts": 1,
"accelerator-type-uuid": "d8424dc4-a4a3-c413-f89e-433683f9040b"
})mdata";
  json_object *j_cluster;

  j_cluster = parse(mdata);
  ASSERT_NE(j_cluster, nullptr);

  opae_metadata_accelerator_cluster_v1 cluster;
  memset(&cluster, 0, sizeof(cluster));

  EXPECT_EQ(opae_bitstream_parse_accelerator_cluster_v1(j_cluster,
							&cluster),
            FPGA_EXCEPTION);

  EXPECT_EQ(cluster.total_contexts, 1);
  EXPECT_EQ(cluster.name, nullptr);
  EXPECT_EQ(cluster.accelerator_type_uuid, nullptr);
}

/**
 * @test       cluster_err2
 * @brief      Test: opae_bitstream_parse_accelerator_cluster_v1
 * @details    If the given json_object contains no,<br>
 *             "accelerator-type-uuid" key,<br>
 *             the fn returns FPGA_EXCEPTION.<br>
 */
TEST_P(metadatav1_c_p, cluster_err2) {
    const char *mdata =
    R"mdata({
"total-contexts": 1,
"name": "nlb_400"
})mdata";
  json_object *j_cluster;

  j_cluster = parse(mdata);
  ASSERT_NE(j_cluster, nullptr);

  opae_metadata_accelerator_cluster_v1 cluster;
  memset(&cluster, 0, sizeof(cluster));

  EXPECT_EQ(opae_bitstream_parse_accelerator_cluster_v1(j_cluster,
							&cluster),
            FPGA_EXCEPTION);

  EXPECT_EQ(cluster.total_contexts, 1);
  EXPECT_EQ(cluster.name, nullptr);
  EXPECT_EQ(cluster.accelerator_type_uuid, nullptr);
}

/**
 * @test       image_err0
 * @brief      Test: opae_bitstream_parse_afu_image_v1
 * @details    If the given json_object contains no,<br>
 *             "clock-frequency-high" key,<br>
 *             the fn returns FPGA_OK.<br>
 */
TEST_P(metadatav1_c_p, image_err0) {
    const char *mdata =
    R"mdata({
"clock-frequency-low": 156,
"power": 50,
"interface-uuid": "01234567-89AB-CDEF-0123-456789ABCDEF",
"magic-no": 488605312,

"accelerator-clusters": [
  {
    "total-contexts": 1,
    "name": "nlb_400",
    "accelerator-type-uuid": "d8424dc4-a4a3-c413-f89e-433683f9040b"
  }
]
})mdata";
  json_object *j_afu_image;

  j_afu_image = parse(mdata);
  ASSERT_NE(j_afu_image, nullptr);

  opae_metadata_afu_image_v1 img;
  memset(&img, 0, sizeof(img));

  fpga_guid ifc_id;

  EXPECT_EQ(opae_bitstream_parse_afu_image_v1(j_afu_image,
					      &img,
					      ifc_id),
            FPGA_OK);

  free(img.interface_uuid);
  free(img.accelerator_clusters[0].name);
  free(img.accelerator_clusters[0].accelerator_type_uuid);
  free(img.accelerator_clusters);
}

/**
 * @test       image_err1
 * @brief      Test: opae_bitstream_parse_afu_image_v1
 * @details    If the given json_object contains no,<br>
 *             "clock-frequency-low" key,<br>
 *             the fn returns FPGA_OK.<br>
 */
TEST_P(metadatav1_c_p, image_err1) {
    const char *mdata =
    R"mdata({
"clock-frequency-high": 31.2,
"power": 50,
"interface-uuid": "01234567-89AB-CDEF-0123-456789ABCDEF",
"magic-no": 488605312,

"accelerator-clusters": [
  {
    "total-contexts": 1,
    "name": "nlb_400",
    "accelerator-type-uuid": "d8424dc4-a4a3-c413-f89e-433683f9040b"
  }
]
})mdata";
  json_object *j_afu_image;

  j_afu_image = parse(mdata);
  ASSERT_NE(j_afu_image, nullptr);

  opae_metadata_afu_image_v1 img;
  memset(&img, 0, sizeof(img));

  fpga_guid ifc_id;

  EXPECT_EQ(opae_bitstream_parse_afu_image_v1(j_afu_image,
					      &img,
					      ifc_id),
            FPGA_OK);

  EXPECT_EQ(img.clock_frequency_high, 31.2);
  EXPECT_EQ(img.power, 50.0);

  free(img.interface_uuid);
  free(img.accelerator_clusters[0].name);
  free(img.accelerator_clusters[0].accelerator_type_uuid);
  free(img.accelerator_clusters);
}

/**
 * @test       image_err2
 * @brief      Test: opae_bitstream_parse_afu_image_v1
 * @details    If the given json_object contains no,<br>
 *             "power" key,<br>
 *             the fn returns FPGA_OK.<br>
 */
TEST_P(metadatav1_c_p, image_err2) {
    const char *mdata =
    R"mdata({
"clock-frequency-high": 3.12,
"clock-frequency-low": 1.56,
"interface-uuid": "01234567-89AB-CDEF-0123-456789ABCDEF",
"magic-no": 488605312,

"accelerator-clusters": [
  {
    "total-contexts": 1,
    "name": "nlb_400",
    "accelerator-type-uuid": "d8424dc4-a4a3-c413-f89e-433683f9040b"
  }
]
})mdata";
  json_object *j_afu_image;

  j_afu_image = parse(mdata);
  ASSERT_NE(j_afu_image, nullptr);

  opae_metadata_afu_image_v1 img;
  memset(&img, 0, sizeof(img));

  fpga_guid ifc_id;

  EXPECT_EQ(opae_bitstream_parse_afu_image_v1(j_afu_image,
					      &img,
					      ifc_id),
            FPGA_OK);

  EXPECT_EQ(img.clock_frequency_high, 3.12);
  EXPECT_EQ(img.clock_frequency_low, 1.56);

  free(img.interface_uuid);
  free(img.accelerator_clusters[0].name);
  free(img.accelerator_clusters[0].accelerator_type_uuid);
  free(img.accelerator_clusters);
}

/**
 * @test       image_err3
 * @brief      Test: opae_bitstream_parse_afu_image_v1
 * @details    If the given json_object contains no,<br>
 *             "magic-no" key,<br>
 *             the fn returns FPGA_EXCEPTION.<br>
 */
TEST_P(metadatav1_c_p, image_err3) {
    const char *mdata =
    R"mdata({
"clock-frequency-high": 312,
"clock-frequency-low": 156,
"power": 50,
"interface-uuid": "01234567-89AB-CDEF-0123-456789ABCDEF",

"accelerator-clusters": [
  {
    "total-contexts": 1,
    "name": "nlb_400",
    "accelerator-type-uuid": "d8424dc4-a4a3-c413-f89e-433683f9040b"
  }
]
})mdata";
  json_object *j_afu_image;

  j_afu_image = parse(mdata);
  ASSERT_NE(j_afu_image, nullptr);

  opae_metadata_afu_image_v1 img;
  memset(&img, 0, sizeof(img));

  fpga_guid ifc_id;

  EXPECT_EQ(opae_bitstream_parse_afu_image_v1(j_afu_image,
					      &img,
					      ifc_id),
            FPGA_EXCEPTION);

  EXPECT_EQ(img.interface_uuid, nullptr);
  EXPECT_EQ(img.accelerator_clusters, nullptr);
}

/**
 * @test       image_err4
 * @brief      Test: opae_bitstream_parse_afu_image_v1
 * @details    If the given json_object contains,<br>
 *             a "magic-no" key that doesn't match the expected,<br>
 *             the fn returns FPGA_EXCEPTION.<br>
 */
TEST_P(metadatav1_c_p, image_err4) {
    const char *mdata =
    R"mdata({
"clock-frequency-high": 312,
"clock-frequency-low": 156,
"power": 50,
"interface-uuid": "01234567-89AB-CDEF-0123-456789ABCDEF",
"magic-no": 488605311,

"accelerator-clusters": [
  {
    "total-contexts": 1,
    "name": "nlb_400",
    "accelerator-type-uuid": "d8424dc4-a4a3-c413-f89e-433683f9040b"
  }
]
})mdata";
  json_object *j_afu_image;

  j_afu_image = parse(mdata);
  ASSERT_NE(j_afu_image, nullptr);

  opae_metadata_afu_image_v1 img;
  memset(&img, 0, sizeof(img));

  fpga_guid ifc_id;

  EXPECT_EQ(opae_bitstream_parse_afu_image_v1(j_afu_image,
					      &img,
					      ifc_id),
            FPGA_EXCEPTION);

  EXPECT_EQ(img.interface_uuid, nullptr);
  EXPECT_EQ(img.accelerator_clusters, nullptr);
}

/**
 * @test       image_err5
 * @brief      Test: opae_bitstream_parse_afu_image_v1
 * @details    If the given json_object contains no,<br>
 *             "interface-uuid" key,<br>
 *             the fn returns FPGA_EXCEPTION.<br>
 */
TEST_P(metadatav1_c_p, image_err5) {
    const char *mdata =
    R"mdata({
"clock-frequency-high": 312,
"clock-frequency-low": 156,
"power": 50,
"magic-no": 488605312,

"accelerator-clusters": [
  {
    "total-contexts": 1,
    "name": "nlb_400",
    "accelerator-type-uuid": "d8424dc4-a4a3-c413-f89e-433683f9040b"
  }
]
})mdata";
  json_object *j_afu_image;

  j_afu_image = parse(mdata);
  ASSERT_NE(j_afu_image, nullptr);

  opae_metadata_afu_image_v1 img;
  memset(&img, 0, sizeof(img));

  fpga_guid ifc_id;

  EXPECT_EQ(opae_bitstream_parse_afu_image_v1(j_afu_image,
					      &img,
					      ifc_id),
            FPGA_EXCEPTION);

  EXPECT_EQ(img.interface_uuid, nullptr);
  EXPECT_EQ(img.accelerator_clusters, nullptr);
}

/**
 * @test       image_err6
 * @brief      Test: opae_bitstream_parse_afu_image_v1
 * @details    If the given json_object contains an,<br>
 *             "interface-uuid" key that is not a valid guid,<br>
 *             the fn returns FPGA_EXCEPTION.<br>
 */
TEST_P(metadatav1_c_p, image_err6) {
    const char *mdata =
    R"mdata({
"clock-frequency-high": 312,
"clock-frequency-low": 156,
"power": 50,
"interface-uuid": "not a valid guid",
"magic-no": 488605312,

"accelerator-clusters": [
  {
    "total-contexts": 1,
    "name": "nlb_400",
    "accelerator-type-uuid": "d8424dc4-a4a3-c413-f89e-433683f9040b"
  }
]
})mdata";
  json_object *j_afu_image;

  j_afu_image = parse(mdata);
  ASSERT_NE(j_afu_image, nullptr);

  opae_metadata_afu_image_v1 img;
  memset(&img, 0, sizeof(img));

  fpga_guid ifc_id;

  EXPECT_EQ(opae_bitstream_parse_afu_image_v1(j_afu_image,
					      &img,
					      ifc_id),
            FPGA_EXCEPTION);

  EXPECT_EQ(img.interface_uuid, nullptr);
  EXPECT_EQ(img.accelerator_clusters, nullptr);
}

/**
 * @test       image_err7
 * @brief      Test: opae_bitstream_parse_afu_image_v1
 * @details    If the given json_object contains no<br>
 *             "accelerator-clusters" key,<br>
 *             the fn returns FPGA_EXCEPTION.<br>
 */
TEST_P(metadatav1_c_p, image_err7) {
    const char *mdata =
    R"mdata({
"clock-frequency-high": 312,
"clock-frequency-low": 156,
"power": 50,
"interface-uuid": "01234567-89AB-CDEF-0123-456789ABCDEF",
"magic-no": 488605312,
})mdata";
  json_object *j_afu_image;

  j_afu_image = parse(mdata);
  ASSERT_NE(j_afu_image, nullptr);

  opae_metadata_afu_image_v1 img;
  memset(&img, 0, sizeof(img));

  fpga_guid ifc_id;

  EXPECT_EQ(opae_bitstream_parse_afu_image_v1(j_afu_image,
					      &img,
					      ifc_id),
            FPGA_EXCEPTION);

  EXPECT_EQ(img.interface_uuid, nullptr);
  EXPECT_EQ(img.accelerator_clusters, nullptr);
}

/**
 * @test       image_err8
 * @brief      Test: opae_bitstream_parse_afu_image_v1
 * @details    If the given json_object contains an,<br>
 *             "accelerator-clusters" key that is not an array,<br>
 *             the fn returns FPGA_EXCEPTION.<br>
 */
TEST_P(metadatav1_c_p, image_err8) {
    const char *mdata =
    R"mdata({
"clock-frequency-high": 312,
"clock-frequency-low": 156,
"power": 50,
"interface-uuid": "01234567-89AB-CDEF-0123-456789ABCDEF",
"magic-no": 488605312,

"accelerator-clusters": 3
})mdata";
  json_object *j_afu_image;

  j_afu_image = parse(mdata);
  ASSERT_NE(j_afu_image, nullptr);

  opae_metadata_afu_image_v1 img;
  memset(&img, 0, sizeof(img));

  fpga_guid ifc_id;

  EXPECT_EQ(opae_bitstream_parse_afu_image_v1(j_afu_image,
					      &img,
					      ifc_id),
            FPGA_EXCEPTION);

  EXPECT_EQ(img.interface_uuid, nullptr);
  EXPECT_EQ(img.accelerator_clusters, nullptr);
}

/**
 * @test       image_err10
 * @brief      Test: opae_bitstream_parse_afu_image_v1
 * @details    If an error occurs when parsing<br>
 *             the "accelerator-clusters" array,<br>
 *             the fn frees any successfully-parsed<br>
 *             array entries and propagates the error<br>
 *             code from opae_bitstream_parse_accelerator_cluster_v1.<br>
 */
TEST_P(metadatav1_c_p, image_err10) {
    const char *mdata =
    R"mdata({
"clock-frequency-high": 312,
"clock-frequency-low": 156,
"power": 50,
"interface-uuid": "01234567-89AB-CDEF-0123-456789ABCDEF",
"magic-no": 488605312,

"accelerator-clusters": [
  {
    "total-contexts": 1,
    "name": "nlb_400",
    "accelerator-type-uuid": "d8424dc4-a4a3-c413-f89e-433683f9040b"
  },
  {
    "total-contexts": "not an integer",
    "name": "nlb_400",
    "accelerator-type-uuid": "d8424dc4-a4a3-c413-f89e-433683f9040b"
  }
]
})mdata";
  json_object *j_afu_image;

  j_afu_image = parse(mdata);
  ASSERT_NE(j_afu_image, nullptr);

  opae_metadata_afu_image_v1 img;
  memset(&img, 0, sizeof(img));

  fpga_guid ifc_id;

  EXPECT_EQ(opae_bitstream_parse_afu_image_v1(j_afu_image,
					      &img,
					      ifc_id),
            FPGA_EXCEPTION);

  EXPECT_EQ(img.interface_uuid, nullptr);
  EXPECT_EQ(img.accelerator_clusters, nullptr);
}

/**
 * @test       parse_v1_err1
 * @brief      Test: opae_bitstream_parse_metadata_v1
 * @details    If the given json_object has no<br>
 *             "platform-name" key,<br>
 *             the fn returns a valid v1 metadata object.<br>
 */
TEST_P(metadatav1_c_p, parse_v1_err1) {
    const char *mdata =
    R"mdata({
  "version": 1,
  "afu-image": {
    "clock-frequency-high": 312,
    "clock-frequency-low": 156,
    "power": 50,
    "interface-uuid": "01234567-89AB-CDEF-0123-456789ABCDEF",
    "magic-no": 488605312,

    "accelerator-clusters": [
      {
        "total-contexts": 1,
        "name": "nlb_400",
        "accelerator-type-uuid": "d8424dc4-a4a3-c413-f89e-433683f9040b"
      }
    ]
  }
})mdata";

  json_object *root;

  root = parse(mdata);
  ASSERT_NE(root, nullptr);

  fpga_guid ifc_id;

  opae_bitstream_metadata_v1 *md =
    opae_bitstream_parse_metadata_v1(root, ifc_id);

  ASSERT_NE(md, nullptr);
  opae_bitstream_release_metadata_v1(md);
}

/**
 * @test       parse_v1_err2
 * @brief      Test: opae_bitstream_parse_metadata_v1
 * @details    If the given json_object has no<br>
 *             "afu-image" key,<br>
 *             the fn returns NULL.<br>
 */
TEST_P(metadatav1_c_p, parse_v1_err2) {
    const char *mdata =
    R"mdata({
  "version": 1,
  "platform-name": "DCP"
})mdata";

  json_object *root;

  root = parse(mdata);
  ASSERT_NE(root, nullptr);

  fpga_guid ifc_id;

  EXPECT_EQ(opae_bitstream_parse_metadata_v1(root,
                                             ifc_id),
            nullptr);
}

/**
 * @test       parse_v1_err3
 * @brief      Test: opae_bitstream_parse_metadata_v1
 * @details    If the call to opae_bitstream_parse_afu_image_v1 fails,<br>
 *             the fn returns NULL.<br>
 */
TEST_P(metadatav1_c_p, parse_v1_err3) {
    const char *mdata =
    R"mdata({
  "version": 1,
  "afu-image": {
    "clock-frequency-high": 312,
    "clock-frequency-low": 156,
    "power": 50,
    "interface-uuid": "not a valid guid",
    "magic-no": 488605312,

    "accelerator-clusters": [
      {
        "total-contexts": 1,
        "name": "nlb_400",
        "accelerator-type-uuid": "d8424dc4-a4a3-c413-f89e-433683f9040b"
      }
    ]
  },
  "platform-name": "DCP"
})mdata";

  json_object *root;

  root = parse(mdata);
  ASSERT_NE(root, nullptr);

  fpga_guid ifc_id;

  EXPECT_EQ(opae_bitstream_parse_metadata_v1(root,
                                             ifc_id),
            nullptr);
}

/**
 * @test       parse_v1_ok
 * @brief      Test: opae_bitstream_parse_metadata_v1
 * @details    When successful,<br>
 *             the fn returns places the parsed "interface-uuid" key<br>
 *             into the pr_interface_id parameter,<br>
 *             and returns FPGA_OK.<br>
 */
TEST_P(metadatav1_c_p, parse_v1_ok) {
    const char *mdata =
    R"mdata({
  "version": 1,
  "afu-image": {
    "clock-frequency-high": 312.0,
    "clock-frequency-low": 156,
    "power": 50.2,
    "interface-uuid": "01234567-89AB-CDEF-0123-456789ABCDEF",
    "magic-no": 488605312,

    "accelerator-clusters": [
      {
        "total-contexts": 1,
        "name": "nlb_400",
        "accelerator-type-uuid": "d8424dc4-a4a3-c413-f89e-433683f9040b"
      },
      {
        "total-contexts": 2,
        "name": "nlb_400",
        "accelerator-type-uuid": "d8424dc4-a4a3-c413-f89e-433683f9040b"
      },
      {
        "total-contexts": 3,
        "name": "nlb_400",
        "accelerator-type-uuid": "d8424dc4-a4a3-c413-f89e-433683f9040b"
      }
    ]
  },
  "platform-name": "DCP"
})mdata";

  fpga_guid expected_id = {
    0x01, 0x23, 0x45, 0x67,
    0x89, 0xab,
    0xcd, 0xef,
    0x01, 0x23,
    0x45, 0x67, 0x89, 0xab, 0xcd, 0xef
  };

  json_object *root;

  root = parse(mdata);
  ASSERT_NE(root, nullptr);

  fpga_guid ifc_id = { 0, };

  opae_bitstream_metadata_v1 *md = 
    opae_bitstream_parse_metadata_v1(root, ifc_id);

  EXPECT_EQ(memcmp(ifc_id, expected_id, sizeof(fpga_guid)), 0);

  ASSERT_NE(md, nullptr);
  EXPECT_EQ(md->version, 1);

  ASSERT_NE(md->platform_name, nullptr);
  EXPECT_STREQ(md->platform_name, "DCP");

  EXPECT_EQ(md->afu_image.clock_frequency_high, 312.0);
  EXPECT_EQ(md->afu_image.clock_frequency_low, 156.0);
  EXPECT_EQ(md->afu_image.power, 50.2);

  ASSERT_NE(md->afu_image.interface_uuid, nullptr);
  EXPECT_STREQ(md->afu_image.interface_uuid, "01234567-89AB-CDEF-0123-456789ABCDEF");

  EXPECT_EQ(md->afu_image.magic_no, 0x1d1f8680);

  EXPECT_EQ(md->afu_image.num_clusters, 3);

  ASSERT_NE(md->afu_image.accelerator_clusters, nullptr);

  int i;
  for (i = 0 ; i < md->afu_image.num_clusters ; ++i) {
    opae_metadata_accelerator_cluster_v1 *c =
      &md->afu_image.accelerator_clusters[i];

    EXPECT_EQ(c->total_contexts, i + 1);
    ASSERT_NE(c->name, nullptr);
    EXPECT_STREQ(c->name, "nlb_400");
    ASSERT_NE(c->accelerator_type_uuid, nullptr);
    EXPECT_STREQ(c->accelerator_type_uuid, "d8424dc4-a4a3-c413-f89e-433683f9040b");
  }

  opae_bitstream_release_metadata_v1(md);
}

INSTANTIATE_TEST_CASE_P(metadatav1_c, metadatav1_c_p,
    ::testing::ValuesIn(test_platform::platforms({})));


class mock_metadatav1_c_p : public metadatav1_c_p {};

/**
 * @test       image_err9
 * @brief      Test: opae_bitstream_parse_afu_image_v1
 * @details    If calloc fails,<br>
 *             the fn returns FPGA_NO_MEMORY.<br>
 */
TEST_P(mock_metadatav1_c_p, image_err9) {
    const char *mdata =
    R"mdata({
"clock-frequency-high": 312,
"clock-frequency-low": 156,
"power": 50,
"interface-uuid": "01234567-89AB-CDEF-0123-456789ABCDEF",
"magic-no": 488605312,

"accelerator-clusters": [
  {
    "total-contexts": 1,
    "name": "nlb_400",
    "accelerator-type-uuid": "d8424dc4-a4a3-c413-f89e-433683f9040b"
  }
]
})mdata";
  json_object *j_afu_image;

  j_afu_image = parse(mdata);
  ASSERT_NE(j_afu_image, nullptr);

  opae_metadata_afu_image_v1 img;
  memset(&img, 0, sizeof(img));

  fpga_guid ifc_id;

  system_->invalidate_calloc(0, "opae_bitstream_parse_afu_image_v1");
  EXPECT_EQ(opae_bitstream_parse_afu_image_v1(j_afu_image,
					      &img,
					      ifc_id),
            FPGA_NO_MEMORY);

  EXPECT_EQ(img.interface_uuid, nullptr);
  EXPECT_EQ(img.accelerator_clusters, nullptr);
}

/**
 * @test       parse_v1_err0
 * @brief      Test: opae_bitstream_parse_metadata_v1
 * @details    If calloc fails,<br>
 *             the fn returns NULL.<br>
 */
TEST_P(mock_metadatav1_c_p, parse_v1_err0) {
    const char *mdata =
    R"mdata({
  "version": 1,
  "afu-image": {
    "clock-frequency-high": 312,
    "clock-frequency-low": 156,
    "power": 50,
    "interface-uuid": "01234567-89AB-CDEF-0123-456789ABCDEF",
    "magic-no": 488605312,

    "accelerator-clusters": [
      {
        "total-contexts": 1,
        "name": "nlb_400",
        "accelerator-type-uuid": "d8424dc4-a4a3-c413-f89e-433683f9040b"
      }
    ]
  },
  "platform-name": "DCP"
})mdata";

  json_object *root;

  root = parse(mdata);
  ASSERT_NE(root, nullptr);

  fpga_guid ifc_id;

  system_->invalidate_calloc(0, "opae_bitstream_parse_metadata_v1");
  EXPECT_EQ(opae_bitstream_parse_metadata_v1(root,
                                             ifc_id),
            nullptr);
}

INSTANTIATE_TEST_CASE_P(metadatav1_c, mock_metadatav1_c_p,
    ::testing::ValuesIn(test_platform::mock_platforms({})));

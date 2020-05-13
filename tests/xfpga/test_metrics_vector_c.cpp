// Copyright(c) 2018-2020, Intel Corporation
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

extern "C" {

#include <json-c/json.h>
#include <uuid/uuid.h>
#include "metrics/vector.h"
#include "opae_int.h"
#include "types_int.h"
}

#include <config.h>
#include <opae/fpga.h>

#include <array>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "gtest/gtest.h"
#include "mock/test_system.h"

using namespace opae::testing;

/**
 * @test       opaec
 * @brief      Tests: fpga_vector_init
 * @details    TODO: test needs a valid comment.
 */
TEST(metric_vector, test_metric_vector_01) {
  fpga_metric_vector vector;

  // NULL Input parameters
  EXPECT_NE(FPGA_OK, fpga_vector_init(NULL));

  // NULL Input parameters
  EXPECT_NE(FPGA_OK, fpga_vector_free(NULL));

  // NULL Input parameters
  EXPECT_NE(FPGA_OK, fpga_vector_total(NULL, NULL));

  // NULL Input parameters
  EXPECT_NE(FPGA_OK, fpga_vector_resize(NULL, 20));

  // Init vector
  EXPECT_EQ(FPGA_OK, fpga_vector_init(&vector));

  // NULL Input parameters
  EXPECT_NE(FPGA_OK, fpga_vector_push(&vector, NULL));

  // NULL Input parameters
  EXPECT_NE(FPGA_OK, fpga_vector_push(NULL, NULL));

  // Delete with NULL
  EXPECT_NE(FPGA_OK, fpga_vector_delete(NULL, 0));

  EXPECT_EQ(FPGA_OK, fpga_vector_free(&vector));
}

/**
 * @test       opaec
 * @brief      Tests: fpga_vector_init
 * @details    TODO: test needs a valid comment.
 */
TEST(metric_vector, test_metric_vector_02) {
  fpga_metric_vector metric_vector;
  struct _fpga_enum_metric *fpga_metric = NULL;

  fpga_metric = (struct _fpga_enum_metric *)malloc(sizeof(struct _fpga_enum_metric));
  EXPECT_EQ(NULL, !fpga_metric);

  // Init vector
  EXPECT_EQ(FPGA_OK, fpga_vector_init(&metric_vector));

  // push item to vector
  EXPECT_EQ(FPGA_OK, fpga_vector_push(&metric_vector, fpga_metric));

  // Delete vector
  EXPECT_NE(FPGA_OK, fpga_vector_delete(NULL, 0));
  EXPECT_NE(FPGA_OK, fpga_vector_delete(&metric_vector, 200));

  // Get NULL vector
  EXPECT_EQ(NULL, fpga_vector_get(NULL, 200));

  // Get item from vector
  EXPECT_EQ(NULL, fpga_vector_get(&metric_vector, 200));

  // free vector
  EXPECT_EQ(FPGA_OK, fpga_vector_free(&metric_vector));
}

/**
 * @test       opaec
 * @brief      Tests: fpga_vector_init
 * @details    TODO: test needs a valid comment.
 */
TEST(metric_vector, test_metric_vector_03) {
  fpga_metric_vector metric_vector;
  uint64_t total;
  // Init vector
  EXPECT_EQ(FPGA_OK, fpga_vector_init(&metric_vector));

  struct _fpga_enum_metric *fpga_enum_metric = NULL;
  fpga_enum_metric = (struct _fpga_enum_metric *)malloc(sizeof(struct _fpga_enum_metric));
  EXPECT_EQ(NULL, !fpga_enum_metric);

  EXPECT_EQ(FPGA_OK, fpga_vector_push(&metric_vector, fpga_enum_metric));

  EXPECT_EQ(FPGA_OK, fpga_vector_total(&metric_vector, &total));

  struct _fpga_enum_metric *fpga_metric_next = NULL;
  fpga_metric_next = (struct _fpga_enum_metric *)calloc(sizeof(struct _fpga_enum_metric), 1);
  EXPECT_EQ(NULL, !fpga_metric_next);

  // push item to vector
  EXPECT_EQ(FPGA_OK, fpga_vector_push(&metric_vector, fpga_metric_next));

  // Get vector
  fpga_vector_get(&metric_vector, 0);

  // Resize vector
  EXPECT_EQ(FPGA_OK, fpga_vector_resize(&metric_vector, 200));

  // free vector
  EXPECT_EQ(FPGA_OK, fpga_vector_free(&metric_vector));
}

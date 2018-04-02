// Copyright(c) 2018, Intel Corporation
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
/*
 * gtArgsFilter.cpp
 */

#include "argsfilter.h"
#include "common_test.h"
#include "gtest/gtest.h"
using namespace common_test;

class argsfilter : public BaseFixture, public ::testing::Test {
 protected:
  virtual void SetUp() {
    m_Properties = NULL;
    EXPECT_EQ(FPGA_OK, fpgaGetProperties(NULL, &m_Properties));
  }

  virtual void TearDown() {
    EXPECT_EQ(FPGA_OK, fpgaDestroyProperties(&m_Properties));
  }

  fpga_properties m_Properties;
};

/**
 * @test       none
 *
 * @brief      Given an argument vector with no options related to
 *             bus, device, function or socket id <br>
 *             When I call set_properties_from_args with that argument
 * vector<br>
 *             Then the return value is 0, the fpga_result object is
 *             unchanged, and my argument vector remains unchanged as well
 *
 */
TEST_F(argsfilter, none) {
  fpga_result res = FPGA_EXCEPTION;
  int argc = 5;
  const char *argv[5] = {"prog", "--opt1", "val1", "word1", "word2"};
  int exit_code = set_properties_from_args(m_Properties, &res, &argc,
                                           const_cast<char **>(argv));
  EXPECT_EQ(exit_code, 0);
  EXPECT_EQ(res, FPGA_EXCEPTION);
  EXPECT_EQ(5, argc);
  EXPECT_STREQ(argv[0], "prog");
  EXPECT_STREQ(argv[1], "--opt1");
  EXPECT_STREQ(argv[2], "val1");
  EXPECT_STREQ(argv[3], "word1");
  EXPECT_STREQ(argv[4], "word2");
}

/**
 * @test       bus_long
 *
 * @brief      Given an argument vector with --bus options set to '0x5e'<br>
 *             When I call set_properties_from_args with that argument
 * vector<br>
 *             Then the return value is 0, the fpga_result object is<br>
 *             FPGA_OK, and my argument vector is changed so that --bus and
 *             0x5e are removed.
 *
 */
TEST_F(argsfilter, bus_long) {
  fpga_result res = FPGA_EXCEPTION;
  int argc = 7;
  uint8_t value = 0;
  const char *argv[7] = {"prog", "--opt1", "val1", "--bus",
                         "0x5e", "word1",  "word2"};
  int exit_code = set_properties_from_args(m_Properties, &res, &argc,
                                           const_cast<char **>(argv));
  EXPECT_EQ(FPGA_OK, fpgaPropertiesGetBus(m_Properties, &value));
  EXPECT_EQ(0x5e, value);
  EXPECT_EQ(exit_code, 0);
  EXPECT_EQ(res, FPGA_OK);
  EXPECT_EQ(5, argc);
  EXPECT_STREQ(argv[0], "prog");
  EXPECT_STREQ(argv[1], "--opt1");
  EXPECT_STREQ(argv[2], "val1");
  EXPECT_STREQ(argv[3], "word1");
  EXPECT_STREQ(argv[4], "word2");
}

/**
 * @test       device_long
 *
 * @brief      Given an argument vector with --device options set to '9'<br>
 *             When I call set_properties_from_args with that argument
 * vector<br>
 *             Then the return value is 0, the fpga_result object is<br>
 *             FPGA_OK, and my argument vector is changed so that --device and
 *             0x5e are removed.
 *
 */
TEST_F(argsfilter, device_long) {
  fpga_result res = FPGA_EXCEPTION;
  int argc = 7;
  uint8_t value = 0;
  const char *argv[7] = {"prog", "--opt1", "val1", "--device",
                         "9",    "word1",  "word2"};
  int exit_code = set_properties_from_args(m_Properties, &res, &argc,
                                           const_cast<char **>(argv));
  EXPECT_EQ(FPGA_OK, fpgaPropertiesGetDevice(m_Properties, &value));
  EXPECT_EQ(9, value);
  EXPECT_EQ(exit_code, 0);
  EXPECT_EQ(res, FPGA_OK);
  EXPECT_EQ(5, argc);
  EXPECT_STREQ(argv[0], "prog");
  EXPECT_STREQ(argv[1], "--opt1");
  EXPECT_STREQ(argv[2], "val1");
  EXPECT_STREQ(argv[3], "word1");
  EXPECT_STREQ(argv[4], "word2");
}

/**
 * @test       function_long
 *
 * @brief      Given an argument vector with --function options set to '9'<br>
 *             When I call set_properties_from_args with that argument
 * vector<br>
 *             Then the return value is 0, the fpga_result object is<br>
 *             FPGA_OK, and my argument vector is changed so that --function and
 *             0x5e are removed.
 *
 */
TEST_F(argsfilter, function_long) {
  fpga_result res = FPGA_EXCEPTION;
  int argc = 7;
  uint8_t value = 0;
  const char *argv[7] = {"prog", "--opt1", "val1", "--function",
                         "1",    "word1",  "word2"};
  int exit_code = set_properties_from_args(m_Properties, &res, &argc,
                                           const_cast<char **>(argv));
  EXPECT_EQ(FPGA_OK, fpgaPropertiesGetFunction(m_Properties, &value));
  EXPECT_EQ(1, value);
  EXPECT_EQ(exit_code, 0);
  EXPECT_EQ(res, FPGA_OK);
  EXPECT_EQ(5, argc);
  EXPECT_STREQ(argv[0], "prog");
  EXPECT_STREQ(argv[1], "--opt1");
  EXPECT_STREQ(argv[2], "val1");
  EXPECT_STREQ(argv[3], "word1");
  EXPECT_STREQ(argv[4], "word2");
}

/**
 * @test       socket_id_long
 *
 * @brief      Given an argument vector with --socket-id options set to '9'<br>
 *             When I call set_properties_from_args with that argument
 * vector<br>
 *             Then the return value is 0, the fpga_result object is<br>
 *             FPGA_OK, and my argument vector is changed so that --socket-id
 * and
 *             0x5e are removed.
 *
 */
TEST_F(argsfilter, socket_id_long) {
  fpga_result res = FPGA_EXCEPTION;
  int argc = 7;
  uint8_t value = 0;
  const char *argv[7] = {"prog", "--opt1", "val1", "--socket-id",
                         "1",    "word1",  "word2"};
  int exit_code = set_properties_from_args(m_Properties, &res, &argc,
                                           const_cast<char **>(argv));
  EXPECT_EQ(FPGA_OK, fpgaPropertiesGetSocketID(m_Properties, &value));
  EXPECT_EQ(1, value);
  EXPECT_EQ(exit_code, 0);
  EXPECT_EQ(res, FPGA_OK);
  EXPECT_EQ(5, argc);
  EXPECT_STREQ(argv[0], "prog");
  EXPECT_STREQ(argv[1], "--opt1");
  EXPECT_STREQ(argv[2], "val1");
  EXPECT_STREQ(argv[3], "word1");
  EXPECT_STREQ(argv[4], "word2");
}

/**
 * @test       all_long
 *
 * @brief      Given an argument vector with all relavant long options<br>
 *             When I call set_properties_from_args with that argument
 * vector<br>
 *             Then the return value is 0, the fpga_result object is<br>
 *             FPGA_OK, and my argument vector is changed so that those options
 *             are removed and all those properties are set the the expected
 *             values from the argument vector<br>
 *
 */
TEST_F(argsfilter, all_long) {
  fpga_result res = FPGA_EXCEPTION;
  int argc = 15;
  uint8_t value = 0;
  const char *argv[15] = {"prog",  "--opt1",      "val1",     "--bus",
                          "0x5e",  "word1",       "--device", "9",
                          "word2", "--function",  "1",        "--opt2",
                          "val2",  "--socket-id", "1"};
  int exit_code = set_properties_from_args(m_Properties, &res, &argc,
                                           const_cast<char **>(argv));
  EXPECT_EQ(FPGA_OK, fpgaPropertiesGetBus(m_Properties, &value));
  EXPECT_EQ(0x5e, value);
  EXPECT_EQ(FPGA_OK, fpgaPropertiesGetDevice(m_Properties, &value));
  EXPECT_EQ(9, value);
  EXPECT_EQ(FPGA_OK, fpgaPropertiesGetFunction(m_Properties, &value));
  EXPECT_EQ(1, value);
  EXPECT_EQ(FPGA_OK, fpgaPropertiesGetSocketID(m_Properties, &value));
  EXPECT_EQ(1, value);
  EXPECT_EQ(exit_code, 0);
  EXPECT_EQ(res, FPGA_OK);
  EXPECT_EQ(7, argc);
  EXPECT_STREQ(argv[0], "prog");
  EXPECT_STREQ(argv[1], "--opt1");
  EXPECT_STREQ(argv[2], "val1");
  EXPECT_STREQ(argv[3], "word1");
  EXPECT_STREQ(argv[4], "word2");
  EXPECT_STREQ(argv[5], "--opt2");
  EXPECT_STREQ(argv[6], "val2");
}

/**
 * @test       bus_short
 *
 * @brief      Given an argument vector with -B options set to '0x5e'<br>
 *             When I call set_properties_from_args with that argument
 * vector<br>
 *             Then the return value is 0, the fpga_result object is<br>
 *             FPGA_OK, and my argument vector is changed so that -B and
 *             0x5e are removed.
 *
 */
TEST_F(argsfilter, bus_short) {
  fpga_result res = FPGA_EXCEPTION;
  int argc = 7;
  uint8_t value = 0;
  const char *argv[7] = {"prog", "--opt1", "val1", "-B",
                         "0x5e", "word1",  "word2"};
  int exit_code = set_properties_from_args(m_Properties, &res, &argc,
                                           const_cast<char **>(argv));
  EXPECT_EQ(FPGA_OK, fpgaPropertiesGetBus(m_Properties, &value));
  EXPECT_EQ(0x5e, value);
  EXPECT_EQ(exit_code, 0);
  EXPECT_EQ(res, FPGA_OK);
  EXPECT_EQ(5, argc);
  EXPECT_STREQ(argv[0], "prog");
  EXPECT_STREQ(argv[1], "--opt1");
  EXPECT_STREQ(argv[2], "val1");
  EXPECT_STREQ(argv[3], "word1");
  EXPECT_STREQ(argv[4], "word2");
}

/**
 * @test       device_short
 *
 * @brief      Given an argument vector with -D options set to '9'<br>
 *             When I call set_properties_from_args with that argument
 * vector<br>
 *             Then the return value is 0, the fpga_result object is<br>
 *             FPGA_OK, and my argument vector is changed so that -D and
 *             0x5e are removed.
 *
 */
TEST_F(argsfilter, device_short) {
  fpga_result res = FPGA_EXCEPTION;
  int argc = 7;
  uint8_t value = 0;
  const char *argv[7] = {"prog", "--opt1", "val1", "-D", "9", "word1", "word2"};
  int exit_code = set_properties_from_args(m_Properties, &res, &argc,
                                           const_cast<char **>(argv));
  EXPECT_EQ(FPGA_OK, fpgaPropertiesGetDevice(m_Properties, &value));
  EXPECT_EQ(9, value);
  EXPECT_EQ(exit_code, 0);
  EXPECT_EQ(res, FPGA_OK);
  EXPECT_EQ(5, argc);
  EXPECT_STREQ(argv[0], "prog");
  EXPECT_STREQ(argv[1], "--opt1");
  EXPECT_STREQ(argv[2], "val1");
  EXPECT_STREQ(argv[3], "word1");
  EXPECT_STREQ(argv[4], "word2");
}

/**
 * @test       function_short
 *
 * @brief      Given an argument vector with -F options set to '9'<br>
 *             When I call set_properties_from_args with that argument
 * vector<br>
 *             Then the return value is 0, the fpga_result object is<br>
 *             FPGA_OK, and my argument vector is changed so that -F and
 *             0x5e are removed.
 *
 */
TEST_F(argsfilter, function_short) {
  fpga_result res = FPGA_EXCEPTION;
  int argc = 7;
  uint8_t value = 0;
  const char *argv[7] = {"prog", "--opt1", "val1", "-F", "1", "word1", "word2"};
  int exit_code = set_properties_from_args(m_Properties, &res, &argc,
                                           const_cast<char **>(argv));
  EXPECT_EQ(FPGA_OK, fpgaPropertiesGetFunction(m_Properties, &value));
  EXPECT_EQ(1, value);
  EXPECT_EQ(exit_code, 0);
  EXPECT_EQ(res, FPGA_OK);
  EXPECT_EQ(5, argc);
  EXPECT_STREQ(argv[0], "prog");
  EXPECT_STREQ(argv[1], "--opt1");
  EXPECT_STREQ(argv[2], "val1");
  EXPECT_STREQ(argv[3], "word1");
  EXPECT_STREQ(argv[4], "word2");
}

/**
 * @test       socket_id_short
 *
 * @brief      Given an argument vector with -S options set to '9'<br>
 *             When I call set_properties_from_args with that argument
 * vector<br>
 *             Then the return value is 0, the fpga_result object is<br>
 *             FPGA_OK, and my argument vector is changed so that -S and
 *             0x5e are removed.
 *
 */
TEST_F(argsfilter, socket_id_short) {
  fpga_result res = FPGA_EXCEPTION;
  int argc = 7;
  uint8_t value = 0;
  const char *argv[7] = {"prog", "--opt1", "val1", "-S", "1", "word1", "word2"};
  int exit_code = set_properties_from_args(m_Properties, &res, &argc,
                                           const_cast<char **>(argv));
  EXPECT_EQ(FPGA_OK, fpgaPropertiesGetSocketID(m_Properties, &value));
  EXPECT_EQ(1, value);
  EXPECT_EQ(exit_code, 0);
  EXPECT_EQ(res, FPGA_OK);
  EXPECT_EQ(5, argc);
  EXPECT_STREQ(argv[0], "prog");
  EXPECT_STREQ(argv[1], "--opt1");
  EXPECT_STREQ(argv[2], "val1");
  EXPECT_STREQ(argv[3], "word1");
  EXPECT_STREQ(argv[4], "word2");
}

/**
 * @test       all_short
 *
 * @brief      Given an argument vector with all relavant short options<br>
 *             When I call set_properties_from_args with that argument
 * vector<br>
 *             Then the return value is 0, the fpga_result object is<br>
 *             FPGA_OK, and my argument vector is changed so that those options
 *             are removed and all those properties are set the the expected
 *             values from the argument vector<br>
 *
 */
TEST_F(argsfilter, all_short) {
  fpga_result res = FPGA_EXCEPTION;
  int argc = 15;
  uint8_t value = 0;
  const char *argv[15] = {"prog",  "--opt1", "val1", "-B",    "0x5e",
                          "word1", "-D",     "9",    "word2", "-F",
                          "1",     "--opt2", "val2", "-S",    "1"};
  int exit_code = set_properties_from_args(m_Properties, &res, &argc,
                                           const_cast<char **>(argv));
  EXPECT_EQ(FPGA_OK, fpgaPropertiesGetBus(m_Properties, &value));
  EXPECT_EQ(0x5e, value);
  EXPECT_EQ(FPGA_OK, fpgaPropertiesGetDevice(m_Properties, &value));
  EXPECT_EQ(9, value);
  EXPECT_EQ(FPGA_OK, fpgaPropertiesGetFunction(m_Properties, &value));
  EXPECT_EQ(1, value);
  EXPECT_EQ(FPGA_OK, fpgaPropertiesGetSocketID(m_Properties, &value));
  EXPECT_EQ(1, value);
  EXPECT_EQ(exit_code, 0);
  EXPECT_EQ(res, FPGA_OK);
  EXPECT_EQ(7, argc);
  EXPECT_STREQ(argv[0], "prog");
  EXPECT_STREQ(argv[1], "--opt1");
  EXPECT_STREQ(argv[2], "val1");
  EXPECT_STREQ(argv[3], "word1");
  EXPECT_STREQ(argv[4], "word2");
  EXPECT_STREQ(argv[5], "--opt2");
  EXPECT_STREQ(argv[6], "val2");
}

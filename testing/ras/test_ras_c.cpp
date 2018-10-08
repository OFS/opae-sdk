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

#include <opae/fpga.h>
extern "C" {

#include <json-c/json.h>
#include <uuid/uuid.h>

// RAS Command line struct
struct  RASCommandLine
{
  uint32_t          flags;
  #define RASAPP_CMD_FLAG_HELP      0x00000001
  #define RASAPP_CMD_FLAG_VERSION   0x00000002
  #define RASAPP_CMD_PARSE_ERROR    0x00000003
  
  #define RASAPP_CMD_FLAG_BUS       0x00000008
  #define RASAPP_CMD_FLAG_DEV       0x00000010
  #define RASAPP_CMD_FLAG_FUNC      0x00000020
  #define RASAPP_CMD_FLAG_SOCKET    0x00000040
  
  int      bus;
  int      device;
  int      function;
  int      socket;
  bool     print_error;
  bool     catast_error;
  bool     fatal_error;
  bool     nonfatal_error;
  bool     clear_injerror;
  bool     mwaddress_error;
  bool     mraddress_error;
  bool     mwlength_error;
  bool     mrlength_error;
  bool     pagefault_error;
};

void RASAppShowHelp(void);
void print_err(const char*, fpga_result);
int ras_main(int argc, char *argv[]);
fpga_result print_errors(fpga_token, const char*, const char*, int);
fpga_result print_ras_errors(fpga_token);
fpga_result print_port_errors(fpga_token);
fpga_result clear_port_errors(fpga_token);
fpga_result print_pwr_temp(fpga_token);
fpga_result mmio_error(struct RASCommandLine*);
fpga_result page_fault_errors();
fpga_result inject_ras_errors(fpga_token, struct RASCommandLine*);
fpga_result clear_inject_ras_errors(fpga_token, struct RASCommandLine*); 
}

#include <opae/mmio.h>
#include <sys/mman.h>
#include <string>
#include "gtest/gtest.h"
#include "test_system.h"

using namespace opae::testing;

class ras_c_p : public ::testing::TestWithParam<std::string> {
 protected:
  ras_c_p() {}

  virtual void SetUp() override {
    std::string platform_key = GetParam();
    ASSERT_TRUE(test_platform::exists(platform_key));
    platform_ = test_platform::get(platform_key);
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);
  }

  virtual void TearDown() override {
    system_->finalize();
  }

  test_platform platform_;
  test_system *system_;
};

/**
 * @test       help
 * @brief      Test: show_help
 * @details    RASAppShowHelp displays the application help message.<br>
 */
TEST_P(ras_c_p, show_help){
  RASAppShowHelp();
}

/**
 * @test       print_err
 * @brief      Test: invalid_print_error
 * @details    When params to print_err is valid, it displays the std error.<br>
 */
TEST_P(ras_c_p, invalid_print_error){
  const std::string des = "/sys/class/fpga/intel-fpga-dev.01";
  print_err(des.c_str(), FPGA_INVALID_PARAM);
}

/**
 * @test       page_fault_errors
 * @brief      Test: invalid_page_fault_errors
 * @details    When fpga_token is invalid, page_fault_errors returns 
 *             FPGA_INVALID_PARAM.<br>
 */
TEST_P(ras_c_p, invalid_page_fault_errors){
  EXPECT_EQ(FPGA_INVALID_PARAM, page_fault_errors());
}

/**
 * @test       print_token_errors
 * @brief      Test: invalid_print_token_errors
 * @details    When fpga_token is invalid, print_errors returns 
 *             FPGA_INVALID_PARAM.<br>
 */
TEST_P(ras_c_p, invalid_print_token_errors){
  fpga_properties filter = NULL;
  uint32_t num_matches = 0;
  fpga_token tok = nullptr;
  const std::string sysfs_port = "/sys/class/fpga/intel-fpga-dev.0/intel-fpga-port.01/errors/errors";

  EXPECT_EQ(FPGA_INVALID_PARAM, print_errors(tok, sysfs_port.c_str(), nullptr, 0)); 

  EXPECT_EQ(fpgaInitialize(NULL), FPGA_OK);
  EXPECT_EQ(fpgaGetProperties(NULL, &filter), FPGA_OK);
  EXPECT_EQ(fpgaPropertiesSetObjectType(filter, FPGA_ACCELERATOR), FPGA_OK);
  EXPECT_EQ(fpgaEnumerate(&filter, 1, &tok, 1, &num_matches), FPGA_OK);

  // Inject invalid error path
  EXPECT_EQ(FPGA_INVALID_PARAM, print_errors(tok, sysfs_port.c_str(), nullptr, 0)); 

  EXPECT_EQ(FPGA_OK, fpgaDestroyToken(&tok));  
  EXPECT_EQ(FPGA_OK, fpgaDestroyProperties(&filter));  
}

/**
 * @test       inject_ras_errors
 * @brief      Test: invalid_inject_ras_errors
 * @details    When fpga_token is invalid, inject_ras_error returns 
 *             FPGA_INVALID_PARAM.<br>
 */
TEST_P(ras_c_p, invalid_inject_ras_errors){
  fpga_token tok = nullptr;
  struct RASCommandLine rasCmdLine = { 0, -1, -1, -1, -1, false,
                                   false, false, false,false,
                                   false, false, false, false, false};

  EXPECT_EQ(FPGA_INVALID_PARAM, inject_ras_errors(tok, &rasCmdLine)); 
}

/**
 * @test       clear_inject_ras_errors
 * @brief      Test: invalid_clear_inject_ras_errors
 * @details    When fpga_token is invalid, clear_inject_ras_error returns 
 *             FPGA_INVALID_PARAM.<br>
 */
TEST_P(ras_c_p, invalid_clear_inject_ras_errors){
  fpga_token tok = nullptr;
  struct RASCommandLine rasCmdLine = { 0, -1, -1, -1, -1, false,
                                   false, false, false,false,
                                   false, false, false, false, false};

  EXPECT_EQ(FPGA_INVALID_PARAM, clear_inject_ras_errors(tok, &rasCmdLine)); 
}

/**
 * @test       print_pwr_temp
 * @brief      Test: invalid_print_pwr_temp
 * @details    When fpga_token is invalid, print_pwr_temp returns 
 *             FPGA_INVALID_PARAM.<br>
 */
TEST_P(ras_c_p, invalid_print_pwr_temp){
  fpga_token tok = nullptr;
  EXPECT_EQ(FPGA_INVALID_PARAM, print_pwr_temp(tok)); 
}

/**
 * @test       mmio_errors
 * @brief      Test: invalid_mmio_errors
 * @details    When RASCommandLine is invalid, mmio_errors returns 
 *             FPGA_INVALID_PARAM.<br>
 */
TEST_P(ras_c_p, invalid_mmio_errors){
  struct RASCommandLine rasCmdLine;
  EXPECT_EQ(FPGA_INVALID_PARAM, mmio_error(&rasCmdLine)); 
}

/**
 * @test       print_ras_errors
 * @brief      Test: invalid_ras_errors
 * @details    When fpga_token is invalid, print_ras_errors returns 
 *             FPGA_INVALID_PARAM.<br>
 */
TEST_P(ras_c_p, invalid_ras_errors){
  fpga_token tok = nullptr;
  EXPECT_EQ(FPGA_INVALID_PARAM, print_ras_errors(tok));
}

/**
 * @test       print_port_errors
 * @brief      Test: invalid_port_errors
 * @details    When fpga_token is invalid, print_port_errors returns 
 *             FPGA_INVALID_PARAM.<br>
 */
TEST_P(ras_c_p, invalid_print_port_errors){
  fpga_token tok = nullptr;
  EXPECT_EQ(FPGA_INVALID_PARAM, print_port_errors(tok));
}

/**
 * @test       clear_port_errors
 * @brief      Test: invalid_port_errors
 * @details    When fpga_token is invalid, clear_port_errors returns 
 *             FPGA_INVALID_PARAM.<br>
 */
TEST_P(ras_c_p, invalid_clear_port_errors){
  fpga_token tok = nullptr;
  EXPECT_EQ(FPGA_INVALID_PARAM, clear_port_errors(tok));
}

/**
 * @test       main_params_01
 * @brief      Test: ras_main
 * @details    When ras_main is called with an valid command option,<br>
 *             it returns zero.<br>
 */
TEST_P(ras_c_p, main_params_01){
  char zero[20];
  char one[20];
  char two[20];
  char three[20];
  char four[20];
  char five[20];
  char six[20];
  char seven[20];
  char eight[20];
  char nine[20];
  char ten[20];
  char eleven[20];
  char twelve[20];
  char thirteen[20];
  char fourteen[20];

  strcpy(zero, "ras");
  strcpy(one, "-B");
  strcpy(two, "0x5e");
  strcpy(three, "-D");
  strcpy(four, "0x0");
  strcpy(five, "-P");
  strcpy(six, "-Q");
  strcpy(seven, "-R");
  strcpy(eight, "-O");
  strcpy(nine, "-N");
  strcpy(ten, "-C");
  strcpy(eleven, "-E");
  strcpy(twelve, "-G");
  strcpy(thirteen, "-H");
  strcpy(fourteen, "I");

  char *argv[] = { zero, one, two, three, four,
                   five, six, seven, eight, nine,
                   ten, eleven, twelve, thirteen, fourteen };

  EXPECT_EQ(ras_main(15, argv), 0);
}

//TEST_P(ras_c_p, main_params_02){
//  char zero[20];
//  char one[20];
//  char two[20];
//  char three[20];
//  char four[20];
//  char five[20];
//  char six[20];
//  char seven[20];
//  char eight[20];
//  //char nine[20];
//  //char ten[20];
//  //char eleven[20];
//  //char twelve[20];
//  //char thirteen[20];
//  //char fourteen[20];
//  strcpy(zero, "ras");
//  strcpy(one, "-B");
//  strcpy(two, "0x5e");
//  strcpy(three, "-D");
//  strcpy(four, "0x0");
//  strcpy(five, "-F");
//  strcpy(six, "0x0");
//  strcpy(seven, "-S");
//  strcpy(eight, "0x0");
//  //strcpy(nine, "-H");
//  //strcpy(ten, "-I");
//  //strcpy(eleven, "-h");
//  //strcpy(twelve, "-H");
//  //strcpy(thirteen, "-I");
//  //strcpy(fourteen, "");
//
//  char *argv[] = { zero, one, two, three, four,
//                   five, six, seven, eight }; 
//  EXPECT_EQ(ras_main(9, argv), 0);
//}

/**
 * @test       main_invalid
 * @brief      Test: ras_main
 * @details    When ras_main is called with an invalid command option,<br>
 *             it returns non-zero.<br>
 */
TEST_P(ras_c_p, test_main){
  char zero[20];
  char one[20];
  
  strcpy(zero, "ras");
  strcpy(one, "-h");

  char *argv[] = { zero, one };
  EXPECT_NE(ras_main(2, argv), 0);
 
  EXPECT_EQ(ras_main(1, argv), 1);
}

INSTANTIATE_TEST_CASE_P(ras_c, ras_c_p,
                        ::testing::Values(std::string("skx-p-1s")));

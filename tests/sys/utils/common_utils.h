// Copyright(c) 2017-2018, Intel Corporation
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

#ifndef __COMMON_STRESS_H__
#define __COMMON_STRESS_H__

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <uuid/uuid.h>
#include <functional>
#include <map>
#include <random>
#include <string>
#include <string.h>


#include <json-c/json.h>
#ifndef BUILD_ASE
#include "sys/utils/log_sys.h"
#endif
#include <opae/access.h>
#include <opae/fpga.h>
#include <opae/properties.h>
#include <opae/types_enum.h>
#include "sys/utils/types_sys.h"
#include <sys/mman.h>

#include "safe_string/safe_string.h"

////////////////////////////////////////////////////////////////////////////////
#define PROTECTION (PROT_READ | PROT_WRITE)

#ifndef MAP_HUGETLB
#define MAP_HUGETLB 0x40000
#endif
#ifndef MAP_HUGE_SHIFT
#define MAP_HUGE_SHIFT 26
#endif

#define MAP_1G_HUGEPAGE (0x1e << MAP_HUGE_SHIFT) /* 2 ^ 0x1e = 1G */

#ifdef __ia64__
#define ADDR (void*)(0x8000000000000000UL)
#define FLAGS_4K (MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED)
#define FLAGS_2M (FLAGS_4K | MAP_HUGETLB)
#define FLAGS_1G (FLAGS_2M | MAP_1G_HUGEPAGE)
#else
#define ADDR (void*)(0x0UL)
#define FLAGS_4K (MAP_PRIVATE | MAP_ANONYMOUS)
#define FLAGS_2M (FLAGS_4K | MAP_HUGETLB)
#define FLAGS_1G (FLAGS_2M | MAP_1G_HUGEPAGE)
#endif

#define NLB_DSM_SIZE (2 * 1024 * 1024)
#define MAX_NLB_WKSPC_SIZE CACHELINE(32768)
#define NLB_DSM_SIZE1 (2 * 1024 * 1024)
#define NLB_DSM_SIZE_1024 (10 * 1024)

////////////////////////////////////////////////////////////////////////////////

#define ASSERT_NOTNULL(X) ASSERT_NE(NULL, (void*)X)
#define SKX_P_NLB0_AFUID "d8424dc4-a4a3-c413-f89e-433683f9040b"
#define SKX_P_NLB3_AFUID "f7df405c-bd7a-cf72-22f1-44b0b93acd18"

#define FME_SYSFS_THERMAL_MGMT_THRESHOLD1	"thermal_mgmt/threshold1"
#define FME_SYSFS_THERMAL_MGMT_THRESHOLD2	"thermal_mgmt/threshold2"
#define FME_SYSFS_POWER_MGMT_THRESHOLD1		"power_mgmt/threshold1"
#define FME_SYSFS_POWER_MGMT_THRESHOLD2		"power_mgmt/threshold2"

#define CACHELINE_ALIGNED_ADDR(p) ((p) >> LOG2_CL)

#define LPBK1_BUFFER_SIZE		MB(1)
#define LPBK1_BUFFER_ALLOCATION_SIZE	MB(2)
#define LPBK1_DSM_SIZE			MB(2)
#define CSR_SRC_ADDR			0x0120
#define CSR_DST_ADDR			0x0128
#define CSR_CTL				0x0138
#define CSR_CFG				0x0140
#define CSR_NUM_LINES			0x0130
#define DSM_STATUS_TEST_COMPLETE	0x40
#define CSR_AFU_DSM_BASEL		0x0110
#define CSR_AFU_DSM_BASEH		0x0114

// ASE ID
#define ASE_TOKEN_MAGIC    0x46504741544f4b40
static const fpga_guid FPGA_FME_ID = {
			0xbf, 0xaf, 0x2a, 0xe9, 0x4a, 0x52, 0x46, 0xe3, 0x82, 0xfe,
						0x38, 0xf0, 0xf9, 0xe1, 0x77, 0x64
};
static const fpga_guid ASE_GUID = {
			0xd8, 0x42, 0x4d, 0xc4, 0xa4,  0xa3, 0xc4, 0x13, 0xf8,0x9e,
						0x43, 0x36, 0x83, 0xf9, 0x04, 0x0b
};

static const fpga_guid 	WRONG_ASE_GUID = {
			0xd8, 0xd8, 0xd8, 0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

#define PR_INTERFACE_ID "pr/interface_id"
#define FPGA_SLOT 0  // hard-coded until multiple slots are supported

#ifndef CL
#define CL(x) ((x)*64)
#endif  // CL
#ifndef LOG2_CL
#define LOG2_CL 6
#endif  // LOG2_CL
#ifndef MB
#define MB(x) ((x)*1024 * 1024)
#endif  // MB

#define MAX_TOKENS 4
#define MAX_PATH 1024


#define LINE(x)    "__FILE__  + ':' + std::to_string(x)"

/* Type definitions */
typedef struct { uint32_t uint[16]; } cache_line;

/**
 * @brief      Class for global options.
 */
class GlobalOptions {
 public:
  static GlobalOptions& Instance();

  void NumSockets(unsigned s) { m_NumSockets = s; }
  unsigned NumSockets() const { return m_NumSockets; }

  void Bus(signed b) { m_Bus = b; }
  signed Bus() const { return m_Bus; }

  void VM(bool bvm) { m_VM = bvm; }
  bool VM() { return m_VM; }

  void Platform(unsigned p) { m_Platform = p; }
  unsigned Platform() const { return m_Platform; }

 protected:
  static GlobalOptions sm_Instance;

	unsigned m_NumSockets;
	signed m_Bus;
	unsigned m_Platform;
	bool m_VM;

	GlobalOptions() : m_NumSockets(1), m_Bus(0), m_Platform(0), m_VM(false) {}

};

namespace common_utils {

// begin convenience functions for straight C unit tests

inline void token_for_fme0(struct _fpga_token* _tok) {
#ifdef BUILD_ASE
         memcpy(_tok->accelerator_id,FPGA_FME_ID, sizeof(fpga_guid));
	    _tok->magic = ASE_TOKEN_MAGIC;
	    _tok->ase_objtype=FPGA_DEVICE;
#else
  // slot 0 FME
  strncpy_s(_tok->sysfspath, sizeof(_tok->sysfspath),
            SYSFS_FPGA_CLASS_PATH "/intel-fpga-dev.0/intel-fpga-fme.0",
            SYSFS_PATH_MAX - 1);
  strncpy_s(_tok->devpath, sizeof(_tok->devpath),
            FPGA_DEV_PATH "/intel-fpga-fme.0", DEV_PATH_MAX - 1);
  _tok->magic = FPGA_TOKEN_MAGIC;
#endif
};

inline void token_for_afu0(struct _fpga_token* _tok) {
#ifdef BUILD_ASE
            memcpy(_tok->accelerator_id,ASE_GUID, sizeof(fpga_guid));
	   _tok->magic = ASE_TOKEN_MAGIC;
           _tok->ase_objtype=FPGA_ACCELERATOR;
#else
  // slot 0 AFU
  strncpy_s(_tok->sysfspath, sizeof(_tok->sysfspath),
            SYSFS_FPGA_CLASS_PATH "/intel-fpga-dev.0/intel-fpga-port.0",
            SYSFS_PATH_MAX - 1);
  strncpy_s(_tok->devpath, sizeof(_tok->devpath),
            FPGA_DEV_PATH "/intel-fpga-port.0", DEV_PATH_MAX - 1);
  _tok->magic = FPGA_TOKEN_MAGIC;
#endif
};

inline void token_for_invalid(struct _fpga_token* _tok) {
#ifdef BUILD_ASE
            memcpy(_tok->accelerator_id,WRONG_ASE_GUID, sizeof(fpga_guid));
	    _tok->magic = FPGA_TOKEN_MAGIC;
#else
  // slot 0 AFU
  strncpy_s(_tok->sysfspath, sizeof(_tok->sysfspath),
            SYSFS_FPGA_CLASS_PATH "/invalid_token", SYSFS_PATH_MAX - 1);
  strncpy_s(_tok->devpath, sizeof(_tok->devpath),
            FPGA_DEV_PATH "/invalid_token", DEV_PATH_MAX - 1);
  _tok->magic = FPGA_TOKEN_MAGIC;
#endif
};

inline bool token_is_fme0(fpga_token t) {
  struct _fpga_token* _t = (struct _fpga_token*)t;

#ifdef BUILD_ASE
	    if(_t->magic != ASE_TOKEN_MAGIC)
		return 0;
	    else
	        return ((0 == memcmp(_t->accelerator_id,FPGA_FME_ID, sizeof(fpga_guid))));
#else
  int indicator = 0;
  signed retval = 0;

  if (strcmp_s(_t->sysfspath, sizeof(_t->sysfspath),
               SYSFS_FPGA_CLASS_PATH "/intel-fpga-dev.0/intel-fpga-fme.0",
               &indicator)) {
    perror("error in string compare");
  } else {
    retval += indicator;
  }

  if (strcmp_s(_t->devpath, sizeof(_t->sysfspath), "/dev/intel-fpga-fme.0",
               &indicator)) {
    perror("error in string compare");
  } else {
    retval += indicator;
  }
  return (retval == 0);
#endif
};

inline bool token_is_afu0(fpga_token t) {
  struct _fpga_token* _t = (struct _fpga_token*)t;


#ifdef BUILD_ASE
	     if(_t->magic != ASE_TOKEN_MAGIC)
		 return 0;
	     else
	         return ((0 == memcmp(_t->accelerator_id,ASE_GUID, sizeof(fpga_guid))));
#else
  int indicator = 0;
  signed retval = 0;

  if (strcmp_s(_t->sysfspath, sizeof(_t->sysfspath),
               SYSFS_FPGA_CLASS_PATH "/intel-fpga-dev.0/intel-fpga-port.0",
               &indicator)) {
    perror("error in string compare");
  } else {
    retval += indicator;
  }

  if (strcmp_s(_t->devpath, sizeof(_t->sysfspath),
               FPGA_DEV_PATH "/intel-fpga-port.0", &indicator)) {
    perror("error in string compare");
  } else {
    retval += indicator;
  }
  return (retval == 0);
#endif
};

// end of convenience functions for straight C unit tests

/**
 * @brief      Enumeration for configuration parsing.
 */
enum config_enum {
    BITSTREAM_MODE0 = 0,
    BITSTREAM_MODE3,
    BITSTREAM_MODE7,
    BITSTREAM_MMIO,
    BITSTREAM_SIGTAP,
    BITSTREAM_PR07,
    BITSTREAM_PR08,
    BITSTREAM_PR09,
    BITSTREAM_PR10,
    BITSTREAM_PR18,
    BITSTREAM_NO_METADATA,
    BITSTREAM_USR_CLK,
    BITSTREAM_INVLAID_METADATA,
    OPAE_INSTALL_PATH,
    TEST_INSTALL_PATH
};

enum nlbmode { NLB_MODE_0 = 0, NLB_MODE_3 };
enum base { HEX, DEC };

extern std::map<config_enum, char*> config_map;

bool checkReturnCodes(fpga_result result, std::string line);
size_t fillBSBuffer(const char* filename, uint8_t** bsbuffer);

fpga_result loadBitstream(const char* path, fpga_token tok);
uint32_t getAllTokens(fpga_token* toks, fpga_objtype, int cbus = 0,
                      fpga_guid = NULL);
signed exerciseNLB0Function(fpga_token tok);
signed exerciseNLB3Function(fpga_token tok);
signed doExternalNLB(fpga_token tok, nlbmode);
void sayHello(fpga_token tok);
int tryOpen(bool shared, uint8_t bus);
void closePRInterfaceIDHandle();
void fetchConfiguration(const char* path);
void checkIOErrors(const char*, uint64_t);
void printIOError(std::string line);

// read functions determine base by format in sequence
fpga_result sysfs_read_64(const char*, uint64_t*);
fpga_result read_sysfs_value(const char*, uint64_t*, fpga_token);

// write functions determine base by enum param
fpga_result sysfs_write_64(const char*, uint64_t, base);
fpga_result write_sysfs_value(const char*, uint64_t, fpga_token, base);

bool check_path_is_dir(const char*);
bool feature_is_supported(const char*, fpga_token);

/**
 * @brief      Random integer generator
 * @details    Used to generate random integers between a given range. Usage
 *             random<1,100> r; int number = r();
 * @tparam     lo_      the low end of the range
 * @tparam     hi_      the high end of the range
 * @tparam     IntTYpe  the type of integer to generate short, int, long,
 * unsigned,
 */
template <int lo_, int hi_, typename IntType = int>
class Random {
 public:
  /// @brief Random<> constructor
  Random() : gen_(rdev_()), dist_(lo_, hi_) {}

  /**
   * @brief      generates a Random integer
   * @return     a Random number between range lo_ and hi_ */
  int operator()() { return dist_(gen_); }

 private:
  std::random_device rdev_;
  std::mt19937 gen_;
  std::uniform_int_distribution<IntType> dist_;
};

/**
 * @brief      Base fixture class, primarily used to enable fpga
 *             enumeration in gtest.
 */
class BaseFixture {
 public:
  int number_found = 0;
  fpga_token tokens[MAX_TOKENS] = {0};
  int index = 0;

  void TestAllFPGA(fpga_objtype, bool, std::function<void()>,
                   fpga_guid guid = NULL);
};

}  // end namespace

#endif  // __COMMON_STRESS_H__

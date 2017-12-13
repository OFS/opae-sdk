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

#include <json-c/json.h>
#include <log_int.h>
#include <opae/access.h>
#include <opae/fpga.h>
#include <opae/properties.h>
#include <opae/types_enum.h>
#include <sys/mman.h>
#include <types_int.h>

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

#define LINE(x) __FILE__ + ':' + std::to_string(x)

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

namespace common_test {

// begin convenience functions for straight C unit tests

inline void token_for_fme0(struct _fpga_token* _tok) {
	// slot 0 FME
	strncpy_s(_tok->sysfspath, sizeof(_tok->sysfspath),
		  SYSFS_FPGA_CLASS_PATH "/intel-fpga-dev.0/intel-fpga-fme.0",
		  SYSFS_PATH_MAX - 1);
	strncpy_s(_tok->devpath, sizeof(_tok->devpath),
		  FPGA_DEV_PATH "/intel-fpga-fme.0", DEV_PATH_MAX - 1);
	_tok->magic = FPGA_TOKEN_MAGIC;
};

inline void token_for_afu0(struct _fpga_token* _tok) {
	// slot 0 AFU
	strncpy_s(_tok->sysfspath, sizeof(_tok->sysfspath),
		  SYSFS_FPGA_CLASS_PATH "/intel-fpga-dev.0/intel-fpga-port.0",
		  SYSFS_PATH_MAX - 1);
	strncpy_s(_tok->devpath, sizeof(_tok->devpath),
		  FPGA_DEV_PATH "/intel-fpga-port.0", DEV_PATH_MAX - 1);
	_tok->magic = FPGA_TOKEN_MAGIC;
};

inline void token_for_invalid(struct _fpga_token* _tok) {
	// slot 0 AFU
	strncpy_s(_tok->sysfspath, sizeof(_tok->sysfspath),
		  SYSFS_FPGA_CLASS_PATH "/invalid_token", SYSFS_PATH_MAX - 1);
	strncpy_s(_tok->devpath, sizeof(_tok->devpath),
		  FPGA_DEV_PATH "/invalid_token", DEV_PATH_MAX - 1);
	_tok->magic = FPGA_TOKEN_MAGIC;
};

inline bool token_is_fme0(fpga_token t) {
	struct _fpga_token* _t = (struct _fpga_token*)t;

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
};

inline bool token_is_afu0(fpga_token t) {
	struct _fpga_token* _t = (struct _fpga_token*)t;

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
};

// end of convenience functions for straight C unit tests

/**
 * @brief      Enumeration for configuration parsing.
 */
enum config_enum {
	BITSTREAM_MODE0 = 0,
	BITSTREAM_MODE3,
	OPAE_INSTALL_PATH
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

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

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <unistd.h>
#include <csignal>
#include <fstream>
#include <thread>
#include "gtest/gtest.h"
#include <opae/access.h>
#include <opae/enum.h>
#include <opae/manage.h>
#include <opae/manage.h>
#include <opae/mmio.h>
#include <opae/properties.h>
#include <opae/types_enum.h>
#include "common_sys.h"
#include "common_utils.h"
#include "safe_string/safe_string.h"

#define SYSFS_PATH_MAX 256
using namespace std;

int usleep(unsigned);

#define SleepMicro(x) std::this_thread::sleep_for(std::chrono::microseconds(x));
#define DSM_STATUS_TEST_COMPLETE	0x40
#define NLB_TEST_MODE_LPBK1		0x000
#define NUM_MODE0_CLS			40

GlobalOptions GlobalOptions::sm_Instance;
GlobalOptions& GlobalOptions::Instance() { return GlobalOptions::sm_Instance; }

namespace common_utils {

std::map<config_enum, char*> config_map = {
	{BITSTREAM_MODE0, (char*)calloc(MAX_PATH, sizeof(char))},
	{BITSTREAM_MODE3, (char*)calloc(MAX_PATH, sizeof(char))},
	{OPAE_INSTALL_PATH, (char*)calloc(MAX_PATH, sizeof(char))}};

/**
 * @brief      Calls out to the nlb0 C++ sample application.
 *
 * @param[in]  tok   The FPGA device/accelerator token
 *
 * @return     Returns exit value from system API
 */
signed exerciseNLB0Function(fpga_token tok) {
  return doExternalNLB(tok, NLB_MODE_0);
}

/**
 * @brief      Calls out to the nlb3 C++ sample application.
 *
 * @param[in]  tok   The FPGA device/accelerator token
 *
 * @return     Returns exit value from system API.
 */
signed exerciseNLB3Function(fpga_token tok) {
  return doExternalNLB(tok, NLB_MODE_3);
}

/**
 * @brief      Calls out to foapp, for inter-process fpgaOpen tests.
 *
 * @param[in]  shared  Boolean switch to open mode (shared, not shared)
 * @param[in]  bus     The hardware bus
 *
 * @return     Returns the exit value from system API callout to foapp
 *             application.
 */
int tryOpen(bool shared, uint8_t bus) {
  char arguments[MAX_PATH] = {0};
  char temporary_path[MAX_PATH] = {0};
  char *path = NULL;

  char* json_path = config_map[TEST_INSTALL_PATH];
  if ((json_path != NULL) && (json_path[0] == '\0')) {
      char* retval = getcwd(&temporary_path[0], sizeof(temporary_path));
      if (NULL == retval) {
        printIOError(LINE(__LINE__));
        return FPGA_INVALID_PARAM;
      }
      path = temporary_path;

  } else {
     path = json_path;
  }

  if (shared) {
    snprintf(&arguments[0], sizeof(arguments), "%s/build/foapp -b %x -s", path,
             bus);
    return system(arguments);
  }

  snprintf(&arguments[0], sizeof(arguments), "%s/build/foapp -b %x", path, bus);
  return system(arguments);
}

/**
 * @brief      Calls out to NLB applications.
 *
 * @param[in]  tok   The FPGA device/accelerator token
 * @param[in]  mode  The NLB mode:  0 or 3
 *
 * @return     Returns exit value from nlb applications.
 */
signed doExternalNLB(fpga_token tok, nlbmode mode) {
  fpga_properties filter = NULL;
  fpga_guid guid;
  char uuid[strlen(SKX_P_NLB0_AFUID)];

  uint8_t socketid = 0;
  uint8_t bus = 0;
  fpga_objtype otype = FPGA_DEVICE;
  signed retval = -1;

  EXPECT_TRUE(
      checkReturnCodes(fpgaGetProperties(tok, &filter), LINE(__LINE__)));

  // These values are not currently used; they are included for testing
  // and to allow eventual use in a more customized invocation of nlb
  // applications, if needed.

  //******************************************************************//
  EXPECT_TRUE(
      checkReturnCodes(fpgaPropertiesGetGUID(filter, &guid), LINE(__LINE__)));

  if (!GlobalOptions::Instance().VM()) {
    EXPECT_TRUE(checkReturnCodes(fpgaPropertiesGetSocketID(filter, &socketid),
                                 LINE(__LINE__)));
  }

  EXPECT_TRUE(
      checkReturnCodes(fpgaPropertiesGetBus(filter, &bus), LINE(__LINE__)));
  //******************************************************************//

  // else socketid remains zero
  EXPECT_TRUE(checkReturnCodes(fpgaPropertiesGetObjectType(filter, &otype),
                               LINE(__LINE__)));

  uuid_unparse(guid, uuid);

  // setup proper arguments based on nlb mode
  char arguments[MAX_PATH] = {0};
  char nlbapp[64] = {0};
  switch (mode) {
    case NLB_MODE_0:
      snprintf(&nlbapp[0], sizeof(nlbapp), "/bin/nlb0");
      break;
    case NLB_MODE_3:
      snprintf(&nlbapp[0], sizeof(nlbapp), "/bin/nlb3");
      break;
  }

  snprintf(&arguments[0], sizeof(arguments),
           "LD_LIBRARY_PATH=%s/lib %s%s --target fpga --bus-number %u",
           config_map[OPAE_INSTALL_PATH], config_map[OPAE_INSTALL_PATH], nlbapp,
           bus);
  retval = system(arguments);

  if (NULL != filter) {
    checkReturnCodes(fpgaDestroyProperties(&filter), LINE(__LINE__));
  }
  return retval;
}


/**
 * @brief      Determines whether or not the input string is a
 *             directory.
 *
 * @param[in]  path  The path string to test
 *
 * @return     True if directory, false otherwise.
 */
bool check_path_is_dir(const char* path) {
  struct stat statbuf;

  stat(path, &statbuf);

  if (S_ISDIR(statbuf.st_mode)) {
    return true;
  }  // directory

  else {
    return false;
  }  // file
}

/**
 * @brief      Converts return codes to string values.
 *
 * @param[in]  result  The FPGA result to print
 * @param[in]  line    Typically, source file and line number
 *
 * @return     Returns false on error code from OPAE library, true
 *             otherwise.
 */
bool checkReturnCodes(fpga_result result, string line) {
  auto pid = getpid();

  switch (result) {
    case FPGA_OK:
      // cout << "fpga ok\t"
      //      << "pid:  " << pid << endl;
      return true;

    case FPGA_INVALID_PARAM:
      cout << endl
           << "fpga invalid param\t"
           << "pid:  " << pid << " ... " << line << endl;
      return false;
    case FPGA_BUSY:
      cout << endl
           << "fpga busy\t"
           << "pid:  " << pid << " ... " << line << endl;
      return false;
    case FPGA_EXCEPTION:
      cout << endl
           << "fpga exception\t"
           << "pid:  " << pid << " ... " << line << endl;
      // raise(SIGINT);
      return false;
    case FPGA_NOT_FOUND:
      cout << endl
           << "fpga not found\t"
           << "pid:  " << pid << " ... " << line << endl;
      return false;
    case FPGA_NO_MEMORY:
      cout << endl
           << "fpga no memory\t"
           << "pid:  " << pid << " ... " << line << endl;
      return false;
    case FPGA_NOT_SUPPORTED:
      cout << endl
           << "fpga not supported\t"
           << "pid:  " << pid << " ... " << line << endl;
      return false;
    case FPGA_NO_DRIVER:
      cout << endl
           << "fpga no driver\t"
           << "pid:  " << pid << " ... " << line << endl;
      return false;
    case FPGA_NO_DAEMON:
      cout << endl
           << "fpga no daemon\t"
           << "pid:  " << pid << " ... " << line << endl;
      return false;
    case FPGA_NO_ACCESS:
      cout << endl
           << "fpga no access\t"
           << "pid:  " << pid << " ... " << line << endl;
      return false;
    case FPGA_RECONF_ERROR:
      cout << endl
           << "fpga reconf error\t"
           << "pid:  " << pid << " ... " << line << endl;
      return false;
  }

  return true;
}

/**
 * @brief      Fills the bitstream buffer used in the reconfiguration
 *             API.  Note:  The client or consumer of this API frees
 *             the buffer.
 *
 * @param[in]  filename  The file name from which to get the bitstream
 *                       data
 * @param      bsbuffer  The unallocated bitstream buffer
 *
 * @return     Returns the length of the file.
 */
size_t fillBSBuffer(const char* filename, uint8_t** bsbuffer) {
  if ((NULL != filename) && (NULL != bsbuffer)) {
    ifstream gbsfile(filename, ios::binary);
    EXPECT_TRUE(gbsfile.good());
    gbsfile.seekg(0, ios::end);
    size_t bitstream_len = gbsfile.tellg();
    EXPECT_NE(bitstream_len, 0);

    gbsfile.seekg(0, ios::beg);
    *bsbuffer = (uint8_t*)calloc(bitstream_len, sizeof(char));
    EXPECT_TRUE(*bsbuffer != NULL);

    gbsfile.read((char*)*bsbuffer, bitstream_len);
    gbsfile.close();
    return bitstream_len;
  } else {
    std::cout << "parameters cannot be NULL" << endl;
    return 0;
  }
}

/**
 * @brief      Reads from the sysfs path.
 *
 * @param[in]  path  The sysfs path
 * @param[out] u     Out param in which to store the read value
 *
 * @return     Returns OPAE library  success or failure code.
 */
fpga_result sysfs_read_64(const char* path, uint64_t* u) {
  int fd = -1;
  int res = 0;
  char buf[SYSFS_PATH_MAX] = {0};
  int b = 0;

  fd = open(path, O_RDONLY);
  if (fd < 0) {
    printf("open(%s) failed", path);
    return FPGA_NOT_FOUND;
  }

  if ((off_t)-1 == lseek(fd, 0, SEEK_SET)) {
    printf("seek failed");

    goto out_close;
  }

  do {
    res = read(fd, buf + b, sizeof(buf) - b);
    if (res <= 0) {
      printf("Read from %s failed", path);

      goto out_close;
    }
    b += res;
    if ((b > sizeof(buf)) || (b <= 0)) {
      printf("Unexpected size reading from %s", path);

      goto out_close;
    }
  } while (buf[b - 1] != '\n' && buf[b - 1] != '\0' && b < sizeof(buf));

  // erase \n
  buf[b - 1] = 0;

  *u = strtoull(buf, NULL, 0);

  if (close(fd)) {
    perror("close");
  }
  return FPGA_OK;

out_close:
  if (close(fd)) {
    perror("close");
  } else {
    fd = -1;
  }

  return FPGA_NOT_FOUND;
}

/**
 * @brief      Writes value to sysfs.
 *
 * @param[in]  path  The sysfs path
 * @param[in]  u     Value to write
 * @param[in]  B     Base (HEX or DEC)
 *
 * @return     Return OPAE library success or failure code.
 */

fpga_result sysfs_write_64(const char* path, uint64_t u, base B) {
  int res = 0;
  char buf[SYSFS_PATH_MAX] = {0};
  int b = 0;
  fpga_result retval = FPGA_OK;

  int fd = open(path, O_WRONLY);

  if (fd < 0) {
    printf("open: %s", strerror(errno));
    retval = FPGA_NOT_FOUND;
    goto out_close;
  }

  if ((off_t)-1 == lseek(fd, 0, SEEK_SET)) {
    printf("seek: %s", strerror(errno));
    retval = FPGA_NOT_FOUND;
    goto out_close;
  }

  switch (B) {
    // write hex value
    case HEX:
      snprintf(buf, sizeof(buf), "%lx", u);
      break;

    // write dec value
    case DEC:
      snprintf(buf, sizeof(buf), "%ld", u);
      break;
  }

  do {
    res = write(fd, buf + b, sizeof(buf) - b);

    if (res <= 0) {
      printf("Failed to write");
      retval = FPGA_NOT_FOUND;
      goto out_close;
    }

    b += res;

    if (b > sizeof(buf) || b <= 0) {
      printf("Unexpected size reading from %s", path);
      retval = FPGA_NOT_FOUND;
      goto out_close;
    }

  } while (buf[b - 1] != '\n' && buf[b - 1] != '\0' && b < sizeof(buf));

  retval = FPGA_OK;
  goto out_close;

out_close:
  if (close(fd) < 0) {
    perror("close");
  } else {
    fd = -1;
  }
  return retval;
}

#ifndef BUILD_ASE
/**
 * @brief      Reads a sysfs value.
 *
 * @param[in]  feature  The feature being read
 * @param      value    The value being read
 * @param[in]  tok      The FPGA device/accelerator token
 *
 * @return     Returns OPAE library success or failure code.
 */
fpga_result read_sysfs_value(const char* feature, uint64_t* value,
                             fpga_token tok) {
  fpga_result result;
  char path[SYSFS_PATH_MAX];
  memset(path, 0, SYSFS_PATH_MAX);

  strcat_s(path, sizeof(path), ((_fpga_token*)tok)->sysfspath);
  strcat_s(path, sizeof(path), feature);
  checkReturnCodes(result = sysfs_read_64(path, value), LINE(__LINE__));
  return result;
}

/**
 * @brief      Writes a sysfs value.
 *
 * @param[in]  feature  The feature being written to
 * @param[in]  value    The value being written
 * @param[in]  tok      The FPGA device/accelerator token
 * @param[in]  B        The numeric base (HEX or DEC)
 *
 * @return     Returns OPAE library success or failure code.
 */
fpga_result write_sysfs_value(const char* feature, uint64_t value,
                              fpga_token tok, base B) {
  fpga_result result = FPGA_OK;
  char path[SYSFS_PATH_MAX] = {0};

  memset(path, 0, SYSFS_PATH_MAX);

  strcat_s(path, sizeof(path), ((_fpga_token*)tok)->sysfspath);
  strcat_s(path, sizeof(path), feature);

  switch (B) {
    case HEX:
      if (!checkReturnCodes(result = sysfs_write_64(path, value, HEX),
                            LINE(__LINE__))) {
        checkIOErrors(path, value);
      }
      break;

    case DEC:
      if (!checkReturnCodes(result = sysfs_write_64(path, value, DEC),
                            LINE(__LINE__))) {
        checkIOErrors(path, value);
      }
      break;
  }
  return result;
}

/**
 * @brief      Ensures the input feature path exists on the system.
 *
 * @param[in]  feature  The feature being tested
 * @param[in]  tok      The FPGA device/accelerator token
 *
 * @return     True if supported, false otherwise.
 */
bool feature_is_supported(const char* feature, fpga_token tok) {
  bool isDir = false;
  char path[SYSFS_PATH_MAX];
  memset(path, 0, SYSFS_PATH_MAX);

  strcat_s(path, sizeof(path), ((_fpga_token*)tok)->sysfspath);
  strcat_s(path, sizeof(path), feature);
  printf("FEATURE PATH:  %s \n", path);
  EXPECT_TRUE(isDir = check_path_is_dir(path));
  return isDir;
}
#endif

void printIOError(string line) {
  perror("IOERROR:  ");
  cout << ":  " << line << endl;
}

void checkIOErrors(const char* syspath, uint64_t value) {
  struct stat idstat;
  if (0 != (stat(syspath, &idstat))) cout << "File stat failed!!" << endl;
  printf("value:  %lx", value);
  printIOError(LINE(__LINE__));
}

/**
 * @brief      Wraps the reconfigure API for use in test code.
 *
 * @param[in]  path  The path to the bitstream file being loaded
 * @param[in]  tok   The FPGA device/accelerator token
 *
 * @return     Returns OPAE library success or failure code.
 */
fpga_result loadBitstream(const char* path, fpga_token tok) {
#ifndef BUILD_ASE
  if (GlobalOptions::Instance().VM()) {
    return FPGA_OK;
  }

  fpga_result result = FPGA_OK;  // return of reconf API

  // fill bitstream buffer
  uint8_t* bsbuffer = NULL;
  size_t bitstream_len = 0;

  bitstream_len = fillBSBuffer(path, &bsbuffer);
  assert(bsbuffer);

  fpga_handle h = NULL;

  fpga_properties proptemp = NULL;
  fpga_properties tokprop = NULL;
  fpga_token toktemp = NULL;

  EXPECT_TRUE(
      checkReturnCodes(fpgaGetProperties(NULL, &proptemp), LINE(__LINE__)));
  EXPECT_TRUE(
      checkReturnCodes(fpgaGetProperties(tok, &tokprop), LINE(__LINE__)));

  uint8_t socketid = 0;
  EXPECT_TRUE(checkReturnCodes(fpgaPropertiesGetSocketID(tokprop, &socketid),
                               LINE(__LINE__)));
  EXPECT_TRUE(checkReturnCodes(fpgaPropertiesSetSocketID(proptemp, socketid),
                               LINE(__LINE__)));

  uint8_t bus = 0;
  EXPECT_TRUE(
      checkReturnCodes(fpgaPropertiesGetBus(tokprop, &bus), LINE(__LINE__)));
  EXPECT_TRUE(
      checkReturnCodes(fpgaPropertiesSetBus(proptemp, bus), LINE(__LINE__)));

  // set to device for reconfig
  EXPECT_TRUE(checkReturnCodes(
      fpgaPropertiesSetObjectType(proptemp, FPGA_DEVICE), LINE(__LINE__)));

  uint32_t num_matches = 0;
  EXPECT_TRUE(checkReturnCodes(
      fpgaEnumerate(&proptemp, 1, &toktemp, 1, &num_matches), LINE(__LINE__)));

  if (num_matches > 0) {
    if (checkReturnCodes(fpgaOpen(toktemp, &h, 0), LINE(__LINE__))) {
      EXPECT_TRUE(checkReturnCodes(fpgaUpdateProperties(tok, proptemp),
                                   LINE(__LINE__)));

      cout << "PR API:  setting bitstream to - " << path << endl;
      cout << "PR API:  using sysfs path - "
           << ((_fpga_token*)toktemp)->sysfspath << endl;

      // Expect return false (fail) from negative test PR 18 here.
      checkReturnCodes(result = fpgaReconfigureSlot(h, FPGA_SLOT, bsbuffer,
                                                    bitstream_len, 0),
                       LINE(__LINE__));  // this is the only return value
      EXPECT_TRUE(checkReturnCodes(fpgaClose(h), LINE(__LINE__)));
    }
  } else {
    cout << "problem loading bitstream" << endl;
  }

  EXPECT_TRUE(
      checkReturnCodes(fpgaDestroyProperties(&proptemp), LINE(__LINE__)));
  EXPECT_TRUE(
      checkReturnCodes(fpgaDestroyProperties(&tokprop), LINE(__LINE__)));
  EXPECT_TRUE(checkReturnCodes(fpgaDestroyToken(&toktemp), LINE(__LINE__)));
  free(bsbuffer);
  return result;
#else
  cout << "Bit stream path: " << path <<endl;
  return FPGA_OK; // Do nothing for ASE
#endif
}

// bus number shall be passed in from the command line
// else default to socketid of zero unless runing on VM,
// in which case, use NULL filter

/**
 * @brief      Gets all FPGA device/accelerator tokens.
 *
 * @param[out]      toks   The token array to populate
 * @param[in]  otype  The object type of tokens requested
 * @param[in]  cbus   The hardware bus
 * @param[in]  guid   The unique identifier, if object is accelerator type.
 *
 * @return     Number of tokens found/matched.
 */
uint32_t getAllTokens(fpga_token toks[], fpga_objtype otype, int cbus,
                      fpga_guid guid) {
  fpga_properties filter[1] = {0};
  uint32_t num_matches = 0;
  void** enum_param = NULL;
  signed number_filters = 0;

  checkReturnCodes(fpgaGetProperties(NULL, &filter[0]), LINE(__LINE__));

  if (cbus > 0) {
    checkReturnCodes(fpgaPropertiesSetBus(filter[0], (uint8_t)cbus),
                     LINE(__LINE__));
  }

  else {  // set the socket id when bus is default of zero (i.e. unused)
    if (cbus == 0) {  // only when not running on VM
      if (!GlobalOptions::Instance().VM()) {
        checkReturnCodes(fpgaPropertiesSetSocketID(filter[0], 0),
                         LINE(__LINE__));
      }
    }
  }

  // else bus is less than 0, run on all sockets

  switch (otype) {
    case FPGA_DEVICE:
      checkReturnCodes(fpgaPropertiesSetObjectType(filter[0], FPGA_DEVICE),
                       LINE(__LINE__));
      break;

    case FPGA_ACCELERATOR:
      checkReturnCodes(fpgaPropertiesSetObjectType(filter[0], FPGA_ACCELERATOR),
                       LINE(__LINE__));
      if (guid != NULL) {
        checkReturnCodes(fpgaPropertiesSetGUID(filter[0], guid),
                         LINE(__LINE__));
      }
      break;
  }

  if (GlobalOptions::Instance().VM()) {
    enum_param = NULL;
  } else {
    enum_param = &filter[0];
    number_filters = 1;
  }

  checkReturnCodes(
      fpgaEnumerate(enum_param, number_filters, toks, MAX_TOKENS, &num_matches),
      LINE(__LINE__));

  if (NULL != filter[0]) {
    checkReturnCodes(fpgaDestroyProperties(&filter[0]), LINE(__LINE__));
  }
  return num_matches;
}

/**
 * @brief      Core test function that supports enumeration.
 *
 * @param[in]  otype          The object type
 * @param[in]  loadbitstream  Boolean switch to preload or not to
 *                            preload bitstream prior to running test
 *                            code.
 * @param[in]  pStrategy      The test code functor
 * @param[in]  guid           A unique identifier to be used as a filter
 *                            if desired
 */
void BaseFixture::TestAllFPGA(fpga_objtype otype, bool loadbitstream,
                              std::function<void()> pStrategy, fpga_guid guid) {
  fpga_properties filter = NULL;
  assert(pStrategy);

  int cbus = GlobalOptions::Instance().Bus();

  checkReturnCodes(fpgaGetProperties(NULL, &filter), LINE(__LINE__));

  if (guid != NULL) {
    checkReturnCodes(fpgaPropertiesSetGUID(filter, guid), LINE(__LINE__));
  }

  number_found = getAllTokens(tokens, otype, cbus);
  ASSERT_GT(number_found, 0);

  // reset the filter to avoid corruption and/or leak
  EXPECT_EQ(FPGA_OK, fpgaDestroyProperties(&filter));

  for (index = 0; index < number_found; index++) {  // enumeration for loop
    uint8_t bus = 0x0;
    uint8_t socketid = 0;

    // reuse filter pointer to hold properties for output
    // but reset/re-allocate the filter to avoid property value corruption
    checkReturnCodes(fpgaGetProperties(tokens[index], &filter), LINE(__LINE__));

    if (!GlobalOptions::Instance().VM()) {
      EXPECT_TRUE(checkReturnCodes(fpgaPropertiesGetSocketID(filter, &socketid),
                                   LINE(__LINE__)));
      EXPECT_TRUE(
          checkReturnCodes(fpgaPropertiesGetBus(filter, &bus), LINE(__LINE__)));
    }
    // else socketid and bus remain zero
    printf("common: +++++++++++++++++++++++++++++++++\n");
    printf("running on socket:  %d\n", socketid);
    printf("running on bus:  %x\n", bus);
    printf("common: +++++++++++++++++++++++++++++++++\n");

    if (NULL != guid) {
      char uuid[16];
      uuid_unparse(guid, &uuid[0]);
      printf("using guid:  %s\n", uuid);
    }

    if (loadbitstream && !GlobalOptions::Instance().VM()) {
      EXPECT_TRUE(checkReturnCodes(
          loadBitstream(config_map[BITSTREAM_MODE0], tokens[index]),
          LINE(__LINE__)));
    }

    pStrategy();  // RUN THE TEST CODE

    // allow "invalid parameter" to be returned for cases that intentionally
    // malform the token
    checkReturnCodes(fpgaDestroyToken(&tokens[index]), LINE(__LINE__));
    // destroy properties each iteration to avoid a leak
    EXPECT_EQ(FPGA_OK, fpgaDestroyProperties(&filter));
  }
}

/**
 * @brief      Runs a Gtest version of hello_fpga application sample
 *             code.
 *
 * @param[in]  tok   The FPGA token
 *
 * @return     void  OPAE library codes are output to user and tests fail
 *             on error.
 */
void sayHello(fpga_token tok) {
  volatile uint64_t* dsm_ptr = NULL;
  volatile uint64_t* status_ptr = NULL;
  volatile uint64_t* input_ptr = NULL;
  volatile uint64_t* output_ptr = NULL;

  uint64_t dsm_wsid;
  uint64_t input_wsid;
  uint64_t output_wsid;

  fpga_handle h = NULL;

  /* Open accelerator and map MMIO */
  EXPECT_TRUE(checkReturnCodes(fpgaOpen(tok, &h, 0), LINE(__LINE__)));

  EXPECT_TRUE(checkReturnCodes(fpgaMapMMIO(h, 0, NULL), LINE(__LINE__)));

  /* Allocate buffers */
  EXPECT_TRUE(checkReturnCodes(
      fpgaPrepareBuffer(h, LPBK1_DSM_SIZE, (void**)&dsm_ptr, &dsm_wsid, 0),
      LINE(__LINE__)));

  EXPECT_TRUE(
      checkReturnCodes(fpgaPrepareBuffer(h, LPBK1_BUFFER_ALLOCATION_SIZE,
                                         (void**)&input_ptr, &input_wsid, 0),
                       LINE(__LINE__)));

  EXPECT_TRUE(
      checkReturnCodes(fpgaPrepareBuffer(h, LPBK1_BUFFER_ALLOCATION_SIZE,
                                         (void**)&output_ptr, &output_wsid, 0),
                       LINE(__LINE__)));

  printf("Running Test\n");

  /* Initialize buffers */
  memset((void*)dsm_ptr, 0, LPBK1_DSM_SIZE);
  memset((void*)input_ptr, 0xAF, LPBK1_BUFFER_SIZE);
  memset((void*)output_ptr, 0xBE, LPBK1_BUFFER_SIZE);

  cache_line* cl_ptr = (cache_line*)input_ptr;
  for (uint32_t i = 0; i < LPBK1_BUFFER_SIZE / CL(1); ++i) {
    cl_ptr[i].uint[15] = i + 1; /* set the last uint in every cacheline */
  }

  /* Reset accelerator */
  EXPECT_TRUE(checkReturnCodes(fpgaReset(h), LINE(__LINE__)));

  /* Program DMA addresses */
  uint64_t iova;
  EXPECT_TRUE(
      checkReturnCodes(fpgaGetIOAddress(h, dsm_wsid, &iova), LINE(__LINE__)));

  EXPECT_TRUE(checkReturnCodes(fpgaWriteMMIO64(h, 0, CSR_AFU_DSM_BASEL, iova),
                               LINE(__LINE__)));

  EXPECT_TRUE(
      checkReturnCodes(fpgaWriteMMIO32(h, 0, CSR_CTL, 0), LINE(__LINE__)));
  EXPECT_TRUE(
      checkReturnCodes(fpgaWriteMMIO32(h, 0, CSR_CTL, 1), LINE(__LINE__)));

  EXPECT_TRUE(
      checkReturnCodes(fpgaGetIOAddress(h, input_wsid, &iova), LINE(__LINE__)));
  EXPECT_TRUE(checkReturnCodes(
      fpgaWriteMMIO64(h, 0, CSR_SRC_ADDR, CACHELINE_ALIGNED_ADDR(iova)),
      LINE(__LINE__)));

  EXPECT_TRUE(checkReturnCodes(fpgaGetIOAddress(h, output_wsid, &iova),
                               LINE(__LINE__)));
  EXPECT_TRUE(checkReturnCodes(
      fpgaWriteMMIO64(h, 0, CSR_DST_ADDR, CACHELINE_ALIGNED_ADDR(iova)),
      LINE(__LINE__)));

  EXPECT_TRUE(checkReturnCodes(
      fpgaWriteMMIO32(h, 0, CSR_NUM_LINES, LPBK1_BUFFER_SIZE / CL(1)),
      LINE(__LINE__)));
  EXPECT_TRUE(checkReturnCodes(fpgaWriteMMIO32(h, 0, CSR_CFG, 0x42000),
                               LINE(__LINE__)));

  status_ptr = dsm_ptr + DSM_STATUS_TEST_COMPLETE / 8;

  /* Start the test */
  EXPECT_TRUE(
      checkReturnCodes(fpgaWriteMMIO32(h, 0, CSR_CTL, 3), LINE(__LINE__)));

  /* Wait for test completion */
  while (0 == ((*status_ptr) & 0x1)) {
    usleep(100);
  }

  /* Stop the device */
  EXPECT_TRUE(
      checkReturnCodes(fpgaWriteMMIO32(h, 0, CSR_CTL, 7), LINE(__LINE__)));

  /* Check output buffer contents */
  for (uint32_t i = 0; i < LPBK1_BUFFER_SIZE; i++) {
    if (((uint8_t*)output_ptr)[i] != ((uint8_t*)input_ptr)[i]) {
      fprintf(stderr,
              "Output does NOT match input "
              "at offset %i!\n",
              i);
      break;
    }
  }

  printf("Done Running Test\n");

  /* Release buffers */
  EXPECT_TRUE(
      checkReturnCodes(fpgaReleaseBuffer(h, output_wsid), LINE(__LINE__)));
  EXPECT_TRUE(
      checkReturnCodes(fpgaReleaseBuffer(h, input_wsid), LINE(__LINE__)));
  EXPECT_TRUE(checkReturnCodes(fpgaReleaseBuffer(h, dsm_wsid), LINE(__LINE__)));
  EXPECT_TRUE(checkReturnCodes(fpgaUnmapMMIO(h, 0), LINE(__LINE__)));
  EXPECT_TRUE(checkReturnCodes(fpgaClose(h), LINE(__LINE__)));
}

}  // end namespace common_utils

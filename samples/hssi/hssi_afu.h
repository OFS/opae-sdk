// Copyright(c) 2020, Intel Corporation
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
#pragma once
#include <iostream>
#include <string>
#include <sstream>
#include <exception>
#include <glob.h>
#include <time.h>
#include "test_afu.h"

using namespace opae::app;
using namespace opae::fpga::types;

#define ETH_AFU_DFH           0x0000
#define ETH_AFU_ID_L          0x0008
#define ETH_AFU_ID_H          0x0010
#define TRAFFIC_CTRL_CMD      0x0030
#define TRAFFIC_CTRL_DATA     0x0038
#define TRAFFIC_CTRL_PORT_SEL 0x0040
#define AFU_SCRATCHPAD        0x0048

#define READ_CMD              0x00000001ULL
#define WRITE_CMD             0x00000002ULL
#define ACK_TRANS             0x00000004ULL
#define AFU_CMD_SHIFT         32
#define WRITE_DATA_SHIFT      32

#define NO_TIMEOUT            0xffffffffffffffffULL

class hssi_afu : public test_afu {
public:
  hssi_afu()
  : test_afu("hssi")
  {}

  std::string ethernet_interface()
  {
    auto props = properties::get(handle_);
  
    std::ostringstream oss;
    oss << "/sys/bus/pci/devices/" <<
        std::setw(4) << std::setfill('0') << std::hex << props->segment << ":" <<
        std::setw(2) << std::setfill('0') << std::hex << props->bus << ":" << 
        std::setw(2) << std::setfill('0') << std::hex << props->device << "." <<
        std::setw(1) << std::setfill('0') << std::hex << props->function <<
        "/fpga_region/region*/dfl-fme.*/dfl-fme.*.*/net/*";
    
    glob_t gl;
    if (glob(oss.str().c_str(), 0, nullptr, &gl)) {
      if (gl.gl_pathv)
        globfree(&gl);
      return std::string("");
    }
    
    if (gl.gl_pathc > 1)
      std::cerr << "Warning: more than one ethernet interface found." << std::endl;
    
    std::string ifc; 
    if (gl.gl_pathc) {
      ifc = gl.gl_pathv[0];
      size_t pos = ifc.rfind("/") + 1;
      ifc = ifc.substr(pos);
    }
    
    if (gl.gl_pathv)
      globfree(&gl);
  
    return ifc;
  }

  void mbox_write(uint16_t offset, uint32_t data)
  {
    volatile uint8_t *mmio_base = handle_->mmio_ptr(0);

    uint64_t val;
    struct timespec ts;
    uint64_t ticks;
    const uint64_t max_ticks = 1000ULL;

    val = (((uint64_t)data) << WRITE_DATA_SHIFT);
    *((volatile uint64_t *)(mmio_base + TRAFFIC_CTRL_DATA)) = val;

    val = (((uint64_t)offset) << AFU_CMD_SHIFT) | WRITE_CMD;
    *((volatile uint64_t *)(mmio_base + TRAFFIC_CTRL_CMD)) = val;

    ticks = max_ticks;
    ts.tv_sec = 0;
    ts.tv_nsec = 100;
    do
    {
        val = *((volatile uint64_t *)(mmio_base + TRAFFIC_CTRL_CMD));
        if (val & ACK_TRANS)
            break;
        if (nanosleep(&ts, NULL) != -1 &&
            ticks != NO_TIMEOUT) {
            if (!ticks)
                throw std::runtime_error("mbox_write timed out [a]");
            --ticks;
        }
    } while (!(val & ACK_TRANS));

    ticks = max_ticks;
    do
    {
        *((volatile uint64_t *)(mmio_base + TRAFFIC_CTRL_CMD)) = ACK_TRANS;
        val = *((volatile uint64_t *)(mmio_base + TRAFFIC_CTRL_CMD));
        if (!(val & ACK_TRANS))
            break;
        if (nanosleep(&ts, NULL) != -1 &&
            ticks != NO_TIMEOUT) {
            if (!ticks)
                throw std::runtime_error("mbox_write timed out [b]");
            --ticks;
        }
    } while (val & ACK_TRANS); 
  }

  uint32_t mbox_read(uint16_t offset)
  {
    volatile uint8_t *mmio_base = handle_->mmio_ptr(0);
    uint32_t res = 0;

    uint64_t val;
    struct timespec ts;
    uint64_t ticks;
    const uint64_t max_ticks = 1000ULL;

    val = (((uint64_t)offset) << AFU_CMD_SHIFT) | READ_CMD;
    *((volatile uint64_t *)(mmio_base + TRAFFIC_CTRL_CMD)) = val;

    ticks = max_ticks;
    ts.tv_sec = 0;
    ts.tv_nsec = 100;
    do
    {
        val = *((volatile uint64_t *)(mmio_base + TRAFFIC_CTRL_CMD));
        if (val & ACK_TRANS)
            break;
        if (nanosleep(&ts, NULL) != -1 &&
            ticks != NO_TIMEOUT) {
            if (!ticks)
                throw std::runtime_error("mbox_read timed out [a]");
            --ticks;
        }
    } while (!(val & ACK_TRANS));

    val = *(volatile uint64_t *)(mmio_base + TRAFFIC_CTRL_DATA);
    res = (uint32_t)val;

    ticks = max_ticks;
    do
    {
        *((volatile uint64_t *)(mmio_base + TRAFFIC_CTRL_CMD)) = ACK_TRANS;
        val = *((volatile uint64_t *)(mmio_base + TRAFFIC_CTRL_CMD));
        if (!(val & ACK_TRANS))
            break;
        if (nanosleep(&ts, NULL) != -1 &&
            ticks != NO_TIMEOUT) {
            if (!ticks)
                throw std::runtime_error("mbox_read timed out [b]");
            --ticks;
        }
    } while (val & ACK_TRANS);

    return res;
  }

};

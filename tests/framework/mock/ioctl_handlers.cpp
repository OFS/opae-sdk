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
/*
 * ioctl_handlers.cpp
 */
#include <fcntl.h>
#include <linux/ioctl.h>
#include <cstdarg>
#include "intel-fpga.h"
#include "fpga-dfl.h"
#include "test_system.h"

namespace opae {
namespace testing {

template <typename T>
static int validate_argp(mock_object* mock, int request, va_list argp) {
  UNUSED_PARAM(mock);
  UNUSED_PARAM(request);
  T* ptr = va_arg(argp, T*);
  if (ptr->argsz != sizeof(*ptr)) {
    return -1;
  }

  return 0;
}

#define DEFAULT_IOCTL_HANDLER(_REQ, _S)                                        \
  namespace {                                                                  \
  static bool r##_S __attribute__((unused)) =                                  \
      test_system::instance()->default_ioctl_handler(_REQ, validate_argp<_S>); \
  }

template <>
int validate_argp<fpga_port_uafu_irq_set>(mock_object* mock, int request, va_list argp) {
  UNUSED_PARAM(mock);
  UNUSED_PARAM(request);
  fpga_port_uafu_irq_set* ptr = va_arg(argp, fpga_port_uafu_irq_set*);
  if (ptr->argsz != sizeof(*ptr)+(ptr->count*sizeof(int32_t))) {
    return -1;
  }

  return 0;
}


// FPGA DEVICE
DEFAULT_IOCTL_HANDLER(FPGA_FME_PORT_RELEASE, fpga_fme_port_release);
DEFAULT_IOCTL_HANDLER(FPGA_FME_PORT_PR, fpga_fme_port_pr);
DEFAULT_IOCTL_HANDLER(FPGA_FME_PORT_ASSIGN, fpga_fme_port_assign);
DEFAULT_IOCTL_HANDLER(FPGA_FME_GET_INFO, fpga_fme_info);
DEFAULT_IOCTL_HANDLER(FPGA_FME_ERR_SET_IRQ, fpga_fme_err_irq_set);

// FPGA ACCELERATOR
DEFAULT_IOCTL_HANDLER(FPGA_PORT_DMA_MAP, fpga_port_dma_map);
DEFAULT_IOCTL_HANDLER(FPGA_PORT_DMA_UNMAP, fpga_port_dma_unmap);
// DEFAULT_IOCTL_HANDLER(FPGA_PORT_RESET);
DEFAULT_IOCTL_HANDLER(FPGA_PORT_GET_REGION_INFO, fpga_port_region_info);
DEFAULT_IOCTL_HANDLER(FPGA_PORT_GET_INFO, fpga_port_info);
DEFAULT_IOCTL_HANDLER(FPGA_PORT_ERR_SET_IRQ, fpga_port_err_irq_set);
DEFAULT_IOCTL_HANDLER(FPGA_PORT_UAFU_SET_IRQ, fpga_port_uafu_irq_set);
DEFAULT_IOCTL_HANDLER(FPGA_PORT_UMSG_SET_MODE, fpga_port_umsg_cfg);
DEFAULT_IOCTL_HANDLER(FPGA_PORT_UMSG_SET_BASE_ADDR, fpga_port_umsg_base_addr);
// DEFAULT_IOCTL_HANDLER(FPGA_PORT_UMSG_ENABLE);
// DEFAULT_IOCTL_HANDLER(FPGA_PORT_UMSG_DISABLE);

// fpga upstream driver ioctl

// FPGA DEVICE
//DEFAULT_IOCTL_HANDLER(DFL_FPGA_FME_PORT_RELEASE, dfl_fpga_fme_port_release);
DEFAULT_IOCTL_HANDLER(DFL_FPGA_FME_PORT_PR, dfl_fpga_fme_port_pr);
//DEFAULT_IOCTL_HANDLER(DFL_FPGA_FME_PORT_ASSIGN, dfl_fpga_fme_port_assign);


// FPGA ACCELERATOR
DEFAULT_IOCTL_HANDLER(DFL_FPGA_PORT_DMA_MAP, dfl_fpga_port_dma_map);
DEFAULT_IOCTL_HANDLER(DFL_FPGA_PORT_DMA_UNMAP, dfl_fpga_port_dma_unmap);

DEFAULT_IOCTL_HANDLER(DFL_FPGA_PORT_GET_REGION_INFO, dfl_fpga_port_region_info);
DEFAULT_IOCTL_HANDLER(DFL_FPGA_PORT_GET_INFO, dfl_fpga_port_info);

}  // end of namespace testing
}  // end of namespace opae

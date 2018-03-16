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
#pragma once
#include <cstdint>
#include <exception>

#include <opae/types_enum.h>

namespace opae {
namespace fpga {
namespace types {

/** no_daemon exception
 *
 * no_daemon tracks the source line of origin
 * for exceptions thrown when the error code
 * FPGA_NO_DAEMON is returned from a call to
 * an OPAE C API function
 */
class no_daemon : public std::exception {
  private:
    static const std::size_t MAX_EXCEPT = 256;

  public:

    /** no_daemon constructor
     *
     * @param[in] func The function that threw the exception
     * @param[in] loc The file and line number where the exception was thrown
     * exception.
     */
    no_daemon(const char* func, const char* loc) noexcept
    : func_(func),
      loc_(loc),
      msg_("failed with return code FPGA_NO_DAEMON")
    {}

    /** Convert this exception to an informative string.
     *  Print out at least the error code returned.
     */
    virtual const char *what() const noexcept override
    {
      auto err = strncpy_s(buf_, MAX_EXCEPT, msg_, 64);
      if (err) return msg_;

      err = strcat_s(buf_, MAX_EXCEPT, "\nin function: ");
      if (err) return msg_;

      err = strcat_s(buf_, MAX_EXCEPT, func_);
      if (err) return msg_;

      err = strcat_s(buf_, MAX_EXCEPT, "\nat: ");
      if (err) return const_cast<const char*>(buf_);

      err = strcat_s(buf_, MAX_EXCEPT, loc_);
      if (err) return const_cast<const char*>(buf_);

      return const_cast<const char*>(buf_);
    }

    /** Convert this exception to its fpga_result.
     */
    operator fpga_result() const noexcept { return FPGA_NO_DAEMON; }

  protected:
    const char* func_;
    const char* loc_;
    const char* msg_;
    mutable char buf_[MAX_EXCEPT];


};

}  // end of namespace types
}  // end of namespace fpga
}  // end of namespace opae

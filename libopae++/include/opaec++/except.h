// Copyright(c) 2017, Intel Corporation
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
#include <exception>
#include <cstdint>

#include <opae/types_enum.h>

namespace opae
{
namespace fpga
{
namespace types
{

/// Identify a particular line in a source file.
class src_location
{
public:
    /** src_location constructor
     * @param[in] file The source file name, typically __FILE__.
     * @param[in] fn The current function, typically __func__.
     * @param[in] line The current line number, typically __LINE__.
     */
    src_location(const char *file,
                 const char *fn,
                 int line) noexcept;

    /** Retrieve the file name component of the location.
     */
    const char *file() const noexcept;

    /** Retrieve the function name component of the location.
     */
    const char *fn() const noexcept { return fn_; }

    /** Retrieve the line number component of the location.
     */
     int line() const noexcept { return line_; }

private:
    const char *file_;
    const char *fn_;
    int line_;
};

/// Construct a src_location object for the current source line.
#define OPAECXX_HERE opae::fpga::types::src_location(__FILE__, __func__, __LINE__)

class except : public std::exception
{
public:
    static const std::size_t MAX_EXCEPT = 100;

    /** except constructor
     * The fpga_result value is FPGA_EXCEPTION.
     *
     * @param[in] loc Location where the exception was constructed.
     */
    except(src_location loc) noexcept;

    /** except constructor
     *
     * @param[in] res The fpga_result value associated with this exception.
     * @param[in] loc Location where the exception was constructed.
     */
    except(fpga_result res, src_location loc) noexcept;

    /** Convert this except to an informative string.
     */
    virtual const char * what() const noexcept override;

    /** Convert this except to its fpga_result.
     */
    operator fpga_result() const noexcept { return res_; }

protected:
    fpga_result res_;
    src_location loc_;
    mutable char buf_[MAX_EXCEPT];
};

} // end of namespace types
} // end of namespace fpga
} // end of namespace opae


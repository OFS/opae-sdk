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

/// Identify a particular line in a source file.
class src_location {
 public:
  /** src_location constructor
   * @param[in] file The source file name, typically __FILE__.
   * @param[in] fn The current function, typically __func__.
   * @param[in] line The current line number, typically __LINE__.
   */
  src_location(const char *file, const char *fn, int line) noexcept;

  src_location(const src_location &other) noexcept;

  src_location &operator=(const src_location &other) noexcept;

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
#define OPAECXX_HERE \
  opae::fpga::types::src_location(__FILE__, __func__, __LINE__)

/** Generic OPAE exception
 *
 * An except tracks the source line of origin
 * and an optional fpga_result. If no fpga_result
 * is given, then FPGA_EXCEPTION is used.
 */
class except : public std::exception {
 public:
  static const std::size_t MAX_EXCEPT = 256;

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

  /** except constructor
   *
   * @param[in] res The fpga_result value associated with this exception.
   * @param[in] msg The error message as a string
   * @param[in] loc Location where the exception was constructed.
   */
  except(fpga_result res, const char *msg, src_location loc) noexcept;

  /** Convert this except to an informative string.
   */
  virtual const char *what() const noexcept override;

  /** Convert this except to its fpga_result.
   */
  operator fpga_result() const noexcept { return res_; }

 protected:
  fpga_result res_;
  const char *msg_;
  src_location loc_;
  mutable char buf_[MAX_EXCEPT];
};

/** invalid_param exception
 *
 * invalid_param tracks the source line of origin
 * for exceptions thrown when the error code
 * FPGA_INVALID_PARAM is returned from a call to
 * an OPAE C API function
 */
class invalid_param : public except {
 public:
  /** invalid_param constructor
   *
   * @param[in] loc Location where the exception was constructed.
   */
  invalid_param(src_location loc) noexcept
      : except(FPGA_INVALID_PARAM, "failed with return code FPGA_INVALID_PARAM",
               loc) {}
};

/** busy exception
 *
 * busy tracks the source line of origin
 * for exceptions thrown when the error code
 * FPGA_BUSY is returned from a call to
 * an OPAE C API function
 */
class busy : public except {
 public:
  /** busy constructor
   *
   * @param[in] loc Location where the exception was constructed.
   */
  busy(src_location loc) noexcept
      : except(FPGA_BUSY, "failed with return code FPGA_BUSY", loc) {}
};

/** exception exception
 *
 * exception tracks the source line of origin
 * for exceptions thrown when the error code
 * FPGA_EXCEPTION is returned from a call to
 * an OPAE C API function
 */
class exception : public except {
 public:
  /** exception constructor
   *
   * @param[in] loc Location where the exception was constructed.
   */
  exception(src_location loc) noexcept
      : except(FPGA_EXCEPTION, "failed with return code FPGA_EXCEPTION", loc) {}
};

/** not_found exception
 *
 * not_found tracks the source line of origin
 * for exceptions thrown when the error code
 * FPGA_NOT_FOUND is returned from a call to
 * an OPAE C API function
 */
class not_found : public except {
 public:
  /** not_found constructor
   *
   * @param[in] loc Location where the exception was constructed.
   */
  not_found(src_location loc) noexcept
      : except(FPGA_NOT_FOUND, "failed with return code FPGA_NOT_FOUND", loc) {}
};

/** no_memory exception
 *
 * no_memory tracks the source line of origin
 * for exceptions thrown when the error code
 * FPGA_NO_MEMORY is returned from a call to
 * an OPAE C API function
 */
class no_memory : public except {
 public:
  /** no_memory constructor
   *
   * @param[in] loc Location where the exception was constructed.
   */
  no_memory(src_location loc) noexcept
      : except(FPGA_NO_MEMORY, "failed with return code FPGA_NO_MEMORY", loc) {}
};

/** not_supported exception
 *
 * not_supported tracks the source line of origin
 * for exceptions thrown when the error code
 * FPGA_NOT_SUPPORTED is returned from a call to
 * an OPAE C API function
 */
class not_supported : public except {
 public:
  /** not_supported constructor
   *
   * @param[in] loc Location where the exception was constructed.
   */
  not_supported(src_location loc) noexcept
      : except(FPGA_NOT_SUPPORTED, "failed with return code FPGA_NOT_SUPPORTED",
               loc) {}
};

/** no_driver exception
 *
 * no_driver tracks the source line of origin
 * for exceptions thrown when the error code
 * FPGA_NO_DRIVER is returned from a call to
 * an OPAE C API function
 */
class no_driver : public except {
 public:
  /** no_driver constructor
   *
   * @param[in] loc Location where the exception was constructed.
   */
  no_driver(src_location loc) noexcept
      : except(FPGA_NO_DRIVER, "failed with return code FPGA_NO_DRIVER", loc) {}
};

/** no_daemon exception
 *
 * no_daemon tracks the source line of origin
 * for exceptions thrown when the error code
 * FPGA_NO_DAEMON is returned from a call to
 * an OPAE C API function
 */
class no_daemon : public except {
 public:
  /** no_daemon constructor
   *
   * @param[in] loc Location where the exception was constructed.
   */
  no_daemon(src_location loc) noexcept
      : except(FPGA_NO_DAEMON, "failed with return code FPGA_NO_DAEMON", loc) {}
};

/** no_access exception
 *
 * no_access tracks the source line of origin
 * for exceptions thrown when the error code
 * FPGA_NO_ACCESS is returned from a call to
 * an OPAE C API function
 */
class no_access : public except {
 public:
  /** no_access constructor
   *
   * @param[in] loc Location where the exception was constructed.
   */
  no_access(src_location loc) noexcept
      : except(FPGA_NO_ACCESS, "failed with return code FPGA_NO_ACCESS", loc) {}
};

/** reconf_error exception
 *
 * reconf_error tracks the source line of origin
 * for exceptions thrown when the error code
 * FPGA_RECONF_ERROR is returned from a call to
 * an OPAE C API function
 */
class reconf_error : public except {
 public:
  /** reconf_error constructor
   *
   * @param[in] loc Location where the exception was constructed.
   */
  reconf_error(src_location loc) noexcept
      : except(FPGA_RECONF_ERROR, "failed with return code FPGA_RECONF_ERROR",
               loc) {}
};

namespace detail {

/** typedef function pointer that returns bool if result is FPGA_OK
 */
typedef bool (*exception_fn)(fpga_result,
                             const opae::fpga::types::src_location &loc);

/** is_ok is a template function that throws an excpetion of its template
 * argument type
 *  if the result code is not FPGA_OK. Otherwise it returns true.
 */
template <typename T>
constexpr bool is_ok(fpga_result result,
                     const opae::fpga::types::src_location &loc) {
  return result == FPGA_OK ? true : throw T(loc);
}

static exception_fn opae_exceptions[12] = {
    is_ok<opae::fpga::types::invalid_param>,
    is_ok<opae::fpga::types::busy>,
    is_ok<opae::fpga::types::exception>,
    is_ok<opae::fpga::types::not_found>,
    is_ok<opae::fpga::types::no_memory>,
    is_ok<opae::fpga::types::not_supported>,
    is_ok<opae::fpga::types::no_driver>,
    is_ok<opae::fpga::types::no_daemon>,
    is_ok<opae::fpga::types::no_access>,
    is_ok<opae::fpga::types::reconf_error>};

static inline void assert_fpga_ok(fpga_result result,
                                  const opae::fpga::types::src_location &loc) {
  if (result > FPGA_OK && result <= FPGA_RECONF_ERROR)
    // our exception table above starts at invalid_param with index 0
    // but FPGA_INVALID_PARAM is actually enum 1 - let's account for that
    opae_exceptions[result - 1](result, loc);
}

}  // end of namespace detail

/// Macro to check of result is FPGA_OK
/// If not, throw exception that corresponds
/// to the result code
#define ASSERT_FPGA_OK(r)                    \
  opae::fpga::types::detail::assert_fpga_ok( \
      r, opae::fpga::types::src_location(__FILE__, __func__, __LINE__));

}  // end of namespace types
}  // end of namespace fpga
}  // end of namespace opae

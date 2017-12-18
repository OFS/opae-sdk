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
#include <memory>
#include <sstream>

namespace opae {
namespace fpga {
namespace internal {

/**
 * @brief wrapped_stream wraps the logging functionality in libopae-c.
 * It is what is returned from a logger object when it
 * begins a stream sequence. Its destructor is called when the sequence ends.
 */
class wrapped_stream
{
 public:

 /**
  * @brief create a wrapped_stream object with a given level
  *
  * @param level The log level expected by the logger in libopae-c
  * @param new_line Used to determine whether or not a newline is
  *                 automatically appended to the stream
  */
  wrapped_stream(int level, bool new_line = false);

  /**
   * @brief copy constructor of wrapped_stream that copies internal
   *        stream data structures
   *
   * @param other The other wrapped_stream object to copy from
   */
  wrapped_stream(const wrapped_stream & other);

  /**
   * @brief assignment operator of a wrapped_stream that copies internal
   *        stream data structures
   *
   * @param other The other wrapped_stream object to copy from
   *
   * @return A reference to `this` after copying internal data
   */
  wrapped_stream & operator=(const wrapped_stream & other);

  /**
   * @brief wrapped_stream destructor which indicates the end of the stream
   * sequence
   */
  ~wrapped_stream();

  /**
   * @brief Templated stream operator
   *
   * @tparam T The type of the variable being streamed
   * @param v The variable to stream
   *
   * @return A reference to `this`
   */
  template<typename T>
  wrapped_stream& operator<<(const T& v)
  {
      sstream_ << v;
      return *this;
  }

  /**
   * @brief Overload of the stream operator used for stream manipulators
   *
   * @param manip The stream manipulator
   *
   * @return A reference to `this`
   */
  wrapped_stream& operator<<(std::ostream& (*manip)(std::ostream&));
 private:
  std::stringstream sstream_;
  int level_;
  char fmt_[4];
};

/**
 * @brief Named logger object used to format log messages with the process
 * id, the logger name, and the level of the log message
 */
class logger {
 public:
  /**
   * @brief log level enumeration that roughly maps to the log levels in
   * libopae-c
   */
  enum class level : int
  {
      none        = -1,
      fatal       = 0,
      exception   = 10,
      error       = 15,
      warn        = 20,
      info        = 25,
      debug       = 40
  };

  /**
   * @brief named logger constructor
   *
   * @param name The name of the logger to include in log messages
   */
  logger(const std::string & name);

  /**
   * @brief Generic log method that uses loglevel parameter
   *
   * @param l The level of the log message
   * @param str An optional string that may include more contextual
   *            information
   *
   * @return A wrapped_stream object to begin the stream sequence
   */
  wrapped_stream log(level l, std::string str = "");

  /**
   * @brief Log method for debug messages
   *
   * @param str An optional string that may include more contextual
   *            information
   *
   * @return A wrapped_stream object to begin the stream sequence
   */
  wrapped_stream debug(std::string str = "");

  /**
   * @brief Log method for info messages
   *
   * @param str An optional string that may include more contextual
   *            information
   *
   * @return A wrapped_stream object to begin the stream sequence
   */
  wrapped_stream info(std::string str = "");

  /**
   * @brief Log method for warning messages
   *
   * @param str An optional string that may include more contextual
   *            information
   *
   * @return A wrapped_stream object to begin the stream sequence
   */
  wrapped_stream warn(std::string str = "");

  /**
   * @brief Log method for error messages
   *
   * @param str An optional string that may include more contextual
   *            information
   *
   * @return A wrapped_stream object to begin the stream sequence
   */
  wrapped_stream error(std::string str= "");

  /**
   * @brief Log method for exception messages
   *
   * @param str An optional string that may include more contextual
   *            information
   *
   * @return A wrapped_stream object to begin the stream sequence
   */
  wrapped_stream exception(std::string str = "");

  /**
   * @brief Log method for fatal error messages
   *        Meant to be called when detecting a condition that will
   *        terminate program execution
   *
   * @param str An optional string that may include more contextual
   *            information
   *
   * @return A wrapped_stream object to begin the stream sequence
   */
  wrapped_stream fatal(std::string str = "");


 private:
  std::string name_;
  int pid_;
};


}  // end of namespace internal
}  // end of namespace fpga
}  // end of namespace opae

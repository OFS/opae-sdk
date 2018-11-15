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
#include <memory>
#include <vector>

#include <opae/cxx/core/handle.h>
#include <opae/cxx/core/token.h>
#include <opae/types.h>

namespace opae {
namespace fpga {
namespace types {

/** Wraps the OPAE fpga_object primitive.
 * sysobject's are created from a call to fpgaTokenGetObject,
 * fpgaHandleGetObject, or fpgaObjectGetObject
 */
class sysobject {
 public:
  typedef std::shared_ptr<sysobject> ptr_t;

  sysobject() = delete;

  sysobject(const sysobject &o) = delete;

  sysobject &operator=(const sysobject &o) = delete;

  /**
   * @brief Get a sysobject from a token. This will be read-only.
   *
   * @param t[in] Token object representing a resource.
   * @param name[in] An identifier representing an object belonging to a
   * resource represented by the token.
   * @param[in] flags Control behavior of object identification and creation.
   * FPGA_OBJECT_GLOB is used to indicate that the name should be treated as a
   * globbing expression.  FPGA_OBJECT_RECURSE_ONE indicates that subobjects be
   * created for objects one level down from the object identified by name.
   * FPGA_OBJECT_RECURSE_ALL indicates that subobjects be created for all
   * objects below the current object identified by name.
   *
   * @return A shared_ptr to a sysobject instance.
   */
  static sysobject::ptr_t get(token::ptr_t t, const std::string &name,
                              int flags = 0);

  /**
   * @brief Get a sysobject from a handle. This will be read-write.
   *
   * @param h[in] Handle object representing an open resource.
   * @param name[in] An identifier representing an object belonging to a
   * resource represented by the handle.
   * @param[in] flags Control behavior of object identification and creation.
   * FPGA_OBJECT_GLOB is used to indicate that the name should be treated as a
   * globbing expression.  FPGA_OBJECT_RECURSE_ONE indicates that subobjects be
   * created for objects one level down from the object identified by name.
   * FPGA_OBJECT_RECURSE_ALL indicates that subobjects be created for all
   * objects below the current object identified by name.
   *
   * @return A shared_ptr to a sysobject instance.
   */
  static sysobject::ptr_t get(handle::ptr_t h, const std::string &name,
                              int flags = 0);

  /**
   * @brief Get a sysobject froman object. This will be read-write if its
   * parent was created from a handle..
   *
   * @param name[in] An identifier representing an object belonging to this
   * object.
   * @param[in] flags Control behavior of object identification and creation.
   * FPGA_OBJECT_GLOB is used to indicate that the name should be treated as a
   * globbing expression.  FPGA_OBJECT_RECURSE_ONE indicates that subobjects be
   * created for objects one level down from the object identified by name.
   * FPGA_OBJECT_RECURSE_ALL indicates that subobjects be created for all
   * objects. Flags are defaulted to 0 meaning no flags.
   *
   * @return A shared_ptr to a sysobject instance.
   */
  sysobject::ptr_t get(const std::string &name, int flags = 0);

  virtual ~sysobject();

  /**
   * @brief Get the size (in bytes) of the object.
   *
   * @return The number of bytes that the object occupies in memory.
   */
  uint32_t size() const;

  /**
   * @brief Read a 64-bit value from an FPGA object.
   * The value is assumed to be in string format and will be parsed. See flags
   * below for changing that behavior.
   *
   * @param[in] flags Flags that control how the object is read
   * If FPGA_OBJECT_SYNC is used then object will update its buffered copy
   * before retrieving the data. If FPGA_OBJECT_RAW is used, then the data
   * will be read as raw bytes into the uint64_t pointer variable. Flags
   * are defaulted to 0 meaning no flags.
   *
   * @return A 64-bit value from the object.
   */
  uint64_t read64(int flags = 0) const;

  /**
   * @brief Write 64-bit value to an FPGA object.
   * The value will be converted to string before writing. See flags below for
   * changing that behavior.
   *
   * @param[in] value The value to write to the object.
   * @param[in] flags Flags that control how the object is written
   * If FPGA_OBJECT_RAW is used, then the value will be written as raw bytes.
   * Flags are defaulted to 0 meaning no flags.
   *
   * @note This operation will force a sync operation to update its cached
   * buffer
   */
  void write64(uint64_t value, int flags = 0) const;

  /**
   * @brief Get all raw bytes from the object.
   *
   * @param[in]flags Flags that control how object is read
   * If FPGA_OBJECT_SYNC is used then object will update its buffered copy
   * before retrieving the data.
   *
   * @return A vector of all bytes in the object.
   */
  std::vector<uint8_t> bytes(int flags = 0) const;

  /**
   * @brief Get a subset of raw bytes from the object.
   *
   * @param[in]flags Flags that control how object is read
   * If FPGA_OBJECT_SYNC is used then object will update its buffered copy
   * before retrieving the data.
   *
   * @return A vector of size bytes in the object starting at offset.
   */
  std::vector<uint8_t> bytes(uint32_t offset, uint32_t size,
                             int flags = 0) const;

  /** Retrieve the underlying fpga_object primitive.
   */
  fpga_object c_type() const { return sysobject_; }

  /** Retrieve the underlying fpga_object primitive.
   */
  operator fpga_object() const { return sysobject_; }

 private:
  sysobject(fpga_object sysobj, token::ptr_t token, handle::ptr_t hnd);
  fpga_object sysobject_;
  token::ptr_t token_;
  handle::ptr_t handle_;
};

}  // end of namespace types
}  // end of namespace fpga
}  // end of namespace opae

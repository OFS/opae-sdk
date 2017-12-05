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
#include <cstdint>
#include <vector>
#include <initializer_list>
#include <chrono>
#include <thread>

#include <opae/buffer.h>
#include <opaec++/handle.h>

namespace opae
{
namespace fpga
{
namespace types
{

class dma_buffer : public std::enable_shared_from_this<dma_buffer>
{
public:
    typedef std::size_t size_t;
    typedef std::shared_ptr<dma_buffer> ptr_t;

    /** dma_buffer destructor.
     */
    virtual ~dma_buffer();

    /** dma_buffer factory method - allocate a dma_buffer.
     * @param[in] handle The handle used to allocate the buffer.
     * @param[in] len    The length in bytes of the requested buffer.
     * @return A valid dma_buffer smart pointer on success, or an
     * empty smart pointer on failure.
     */
    static dma_buffer::ptr_t allocate(handle::ptr_t handle, size_t len);

    /** Retrieve the virtual address of the buffer base.
     */
    volatile uint8_t * get() const { return virt_; }

    /** Retrieve the handle smart pointer associated with
     * this buffer.
     */
    handle::ptr_t owner() const { return handle_; }

    /** Retrieve the length of the buffer in bytes.
     */
    size_t size() const { return len_; }

    /** Retrieve the address of the buffer suitable for
     * programming into the accelerator device.
     */
    uint64_t iova() const { return iova_; }

    /** Divide a buffer into a series of smaller buffers.
     *
     * For each item sz found in sizes, create a new dma_buffer
     * smart pointer whose size is sz and append the new smart
     * pointer to the end of the returned vector. The sub-buffers
     * are created in increasing order from the beginning of
     * this dma_buffer. The parent buffer (this) of each sub-buffer
     * is tracked so that it cannot be freed prior to a sub-buffer
     * free.
     *
     * @param[in] sizes An initializer list of sizes for the sub-buffers.
     * @return A std::vector of the sub-buffer pointers.
     */
    template <typename T>
    std::vector<ptr_t> split(std::initializer_list<T> sizes)
    {
        std::vector<ptr_t> v;
        size_t offset = 0;

        v.reserve(sizes.size());

        for (const auto &sz : sizes) {
            ptr_t p;
            p.reset(new dma_buffer(handle_,
                                   sz,
                                   virt_ + offset,
                                   wsid_,
                                   iova_ + offset,
                                   shared_from_this()));
            v.push_back(p);
            offset += sz;
        }

        return v;
    }

    /** Write c to each byte location in the buffer.
     */
    void fill(int c);

    /** Compare this dma_buffer (the first len bytes)
     * to that held in other, using memcmp().
     */
    int compare(ptr_t other, size_t len) const;

    /** Read a T-sized block of memory at the given location.
     * @param[in] offset The byte offset from the start of the buffer.
     * @return A T from buffer base + offset.
     */
    template <typename T>
    T read(size_t offset) const
    {
        if ((offset < len_) && (virt_ != nullptr)) {
            return *reinterpret_cast<T *>(virt_ + offset);
        }
        // TODO log/throw error

        return T();
    }

    /** Write a T-sized block of memory to the given location.
     * @param[in] value The value to write.
     * @param[in] offset The byte offset from the start of the buffer.
     */
    template <typename T>
    void write(const T &value, size_t offset)
    {
        if ((offset < len_) && (virt_ != nullptr)) {
            *reinterpret_cast<T *>(virt_ + offset) = value;
        }
        // TODO log/throw error
    }

protected:
    handle::ptr_t handle_;
    size_t len_;
    uint8_t *virt_;
    uint64_t wsid_;
    uint64_t iova_;
    ptr_t parent_; // for split buffers

private:
    dma_buffer(handle::ptr_t handle,
               size_t len,
               uint8_t *virt,
               uint64_t wsid,
               uint64_t iova);

    dma_buffer(handle::ptr_t handle,
               size_t len,
               uint8_t *virt,
               uint64_t wsid,
               uint64_t iova,
               ptr_t parent);
};

/** Poll for a specific value to appear at a given buffer location.
 *
 * Loops on a memory location without explicitly yielding the processor.
 * This API should be used only for time-critical scenarios, eg waiting
 * on a DMA address to be updated by hardware.
 *
 * @param[in] buf The buffer containing the memory location.
 * @param[in] offset The byte offset from the start of the buffer.
 * @param[in] timeout The maximum time to wait in microseconds.
 * @param[in] mask A bit mask to apply to the value read from the buffer.
 * @param[in] value The expected value after applying the mask.
 * @retval true If the expected value was observed.
 * @retval false If the timeout expired before seeing the expected value.
 */
template <typename T>
bool poll(dma_buffer::ptr_t buf,
          size_t offset,
          std::chrono::microseconds timeout,
          T mask,
          T value)
{
    std::chrono::high_resolution_clock::time_point
        start = std::chrono::high_resolution_clock::now();

    std::chrono::microseconds elapsed;

    do
    {
        if ((buf->read<T>(offset) & mask) == value)
            return true;

        elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::high_resolution_clock::now() - start);

    }while(elapsed < timeout);

    return false;
}

/** Wait for a specific value to appear at a given buffer location.
 *
 * Loops on a memory location, yielding the processor after each iteration.
 * This API should not be used for time-critical scenarios. See poll instead.
 *
 * @param[in] buf The buffer containing the memory location.
 * @param[in] offset The byte offset from the start of the buffer.
 * @param[in] each The number of microseconds to sleep each loop iteration.
 * @param[in] timeout The maximum time to wait in microseconds.
 * @param[in] mask A bit mask to apply to the value read from the buffer.
 * @param[in] value The expected value after applying the mask.
 * @retval true If the expected value was observed.
 * @retval false If the timeout expired before seeing the expected value.
 */
template <typename T>
bool wait(dma_buffer::ptr_t buf,
          size_t offset,
          std::chrono::microseconds each,
          std::chrono::microseconds timeout,
          T mask,
          T value)
{
    std::chrono::high_resolution_clock::time_point
        start = std::chrono::high_resolution_clock::now();
    std::chrono::microseconds elapsed;

    do
    {
        if ((buf->read<T>(offset) & mask) == value)
            return true;

        std::this_thread::sleep_for(each);

        elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::high_resolution_clock::now() - start);

    }while(elapsed < timeout);

    return false;
}

} // end of namespace types
} // end of namespace fpga
} // end of namespace opae

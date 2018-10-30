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
#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include "fme.h"


using namespace std;


namespace intel
{
namespace fpga
{

fme::fme(const string & resource, bool read_only)
    : mmio_(0),
      resource_(resource),
      fd_(-1),
      mmap_size_(0)
{
    struct stat stbuffer;
    int flags = read_only ?  O_RDONLY : O_RDWR | O_SYNC;
    fd_ = ::open(resource_.c_str(), flags);
    if (fd_ > 0 && ::fstat(fd_ , &stbuffer) == 0)
    {
        mmap_size_ = stbuffer.st_size;
        int prot = read_only ? PROT_READ : PROT_READ | PROT_WRITE;
        mmio_ = mmap(0, mmap_size_, prot, MAP_SHARED, fd_, 0);
        // if map fails, set mmio_ to 0 as this determins whether
        // the fme device is open in the is_open function
        if (mmio_ == MAP_FAILED)
        {
            mmio_ = 0;
        }
    }
    else
    {
        std::cerr << "Error opening resource: " << resource_ << "\n";
    }
}

fme::~fme()
{
    if (mmio_)
    {
        munmap(mmio_, mmap_size_);
        mmio_ = 0;
    }

    if (fd_ > 0)
    {
        close(fd_);
        fd_ = 0;
    }
}

volatile uint64_t & fme::operator[](uint32_t offset)
{
    if (static_cast<uint32_t>(offset) >= mmap_size_)
    {
        throw new std::overflow_error("offset goes outside of MMIO space");
    }

    return *reinterpret_cast<volatile uint64_t*>(base_ptr() + offset);
}


fme::ptr_t fme::open(string resource, int8_t socket_id, bool read_only)
{
    ptr_t fme_ptr(0);
    fme_ptr.reset(new fme(resource, read_only));
    if (fme_ptr->mmio_ == nullptr ||
        (socket_id >= 0 &&
         static_cast<uint8_t>(socket_id) != (fme_ptr->ref()[csr::fab_capability] & static_cast<uint8_t>(bitmask::socket_id))))
    {
        fme_ptr.reset();
    }

    return fme_ptr;
}

}   // end of namespace fpga
}   // end of namespace intel

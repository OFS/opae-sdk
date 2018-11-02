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
#pragma once
#include "przone.h"
#include "mmio.h"

namespace intel
{
namespace fpga
{
namespace hssi
{

class hssi_przone : public przone_interface
{
public:
    typedef std::shared_ptr<hssi_przone> ptr_t;
    enum class ack_t : uint64_t
    {
        nack = 0,
        ack
    };

    enum class fme_csr : uint32_t
    {
        dfh_start    = 0x00,
        bitstream_id = 0x60,
        hssi_ctrl    = 0x88,
        hssi_stat    = 0x90,
    };


    hssi_przone(mmio::ptr_t mmio, uint32_t ctrl, uint32_t stat);
    virtual ~hssi_przone(){}

    virtual bool read(uint32_t address, uint32_t & value);
    virtual bool write(uint32_t address, uint32_t value);

    /// @brief Perform the HSSI acknowledge routine
    ///        This will wait for an ack message, then write 0 to the HSSI_CTRL register.
    ///        Finally, this will wait for a nack message
    ///
    /// @return true if the routine completed successfully, false if any of the waits timed out
    bool hssi_ack(uint32_t timeout_usec = 1000, uint32_t * duration = 0);
    bool wait_for_ack(ack_t response, uint32_t timeout_usec = 1000, uint32_t * duration = 0);

    uint32_t get_ctrl() const;
    uint32_t get_stat() const;
    mmio::ptr_t get_mmio() const;

private:

    static const uint32_t ack_bit = 32;

    mmio::ptr_t mmio_;
    uint32_t ctrl_;
    uint32_t stat_;

};

} // end of namespace hssi
} // end of namespace fpga
} // end of namespace intel



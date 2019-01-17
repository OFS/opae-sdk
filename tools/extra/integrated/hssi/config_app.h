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
#include <sstream>
#include "przone.h"
#include "i2c.h"
#include "mdio.h"
#include "option_map.h"
#include "cmd_handler.h"
#include "log.h"
#include "mmio.h"

namespace intel
{
namespace fpga
{
namespace hssi
{

enum class eq_register_type : uint8_t
{
    unknown = 0,
    fpga_tx,
    fpga_rx,
    retimer_tx,
    retimer_rx,
    hssi_mode,
    mdio,
    przone
};

struct eq_register
{
    eq_register_type type;
    int32_t channel_lane;
    int32_t device;
    uint32_t address;
    uint32_t value;
    eq_register()
    : type(eq_register_type::unknown)
    , channel_lane(-1)
    , device(-1)
    , address(0)
    , value(0)
    {}

    eq_register(eq_register_type in_type, int32_t cl, int32_t dev, uint32_t addr, uint32_t in_value=0 )
    : type(in_type)
    , channel_lane(cl)
    , device(dev)
    , address(addr)
    , value(in_value)
    {}
};

class config_app
{
public:
    config_app();

    virtual ~config_app();

    virtual intel::utils::option_map & get_options()
    {
        return options_;
    }

    bool setup();
    void show_help();

    uint32_t run(const std::vector<std::string> & args);
    size_t parse_registers(const std::vector<std::vector<std::string>> & data,
                           std::vector<eq_register> & registers);
    void load(std::istream & stream);
    size_t load(eq_register registers[], size_t size);
    size_t dump(eq_register registers[], size_t size, std::ostream & stream);
    bool hssi_soft_cmd(uint32_t nios_func, std::vector<uint32_t> args);
    bool hssi_soft_cmd(uint32_t nios_func, std::vector<uint32_t> args, uint32_t & value_out);
    bool xcvr_pll_status_read(uint32_t info_sel, uint32_t &value);
    bool xcvr_read(uint32_t lane, uint32_t reg_addr, uint32_t &value);
    bool xcvr_write(uint32_t lane, uint32_t reg_addr, uint32_t value);
    bool retimer_read(uint32_t device_addr, uint8_t channel, uint32_t address, uint32_t & value);
    bool retimer_write(uint32_t device_addr, uint8_t channel, uint32_t address, uint32_t value);



private:
    bool                      c_header_;
    uint32_t                  hssi_cmd_count_;
    uint32_t                  ctrl_;
    uint32_t                  stat_;
    uint32_t                  byte_addr_size_;
    std::ostringstream        header_stream_;
    std::string               input_file_;
    intel::utils::logger      log_;
    mmio::ptr_t               mmio_;
    przone_interface::ptr_t   przone_;
    i2c::ptr_t                i2c_;
    mdio::ptr_t               mdio_;
    intel::utils::option_map  options_;
    intel::utils::cmd_handler console_;

    enum class ack_t : uint64_t
    {
        nack = 0,
        ack
    };

    /// @brief Wait for an ack or nack message from the HSSI controller
    /// @note ack means the ack bit was asserted
    ///       nack means the ack bit was de-asserted
    /// @param[in] response The type of response to poll for (ack or nack)
    /// @param[in] timeout_usec The number of microseconds at which this times out
    /// @param[out] duration Optional output variable that indicates the how long it took
    ///             to receive the message
    ///
    /// @return true if it receives the message beforeeq_register_type the timeout period, false otherwise
    bool wait_for_ack(ack_t response, uint32_t timeout_usec=1000, uint32_t * duration = 0);

    /// @brief Perform the HSSI acknowledge routine
    ///        This will wait for an ack message, then write 0 to the HSSI_CTRL register.
    ///        Finally, this will wait for a nack messaeq_register_typege
    ///
    /// @return true if the routine completed successfully, false if any of the waits timed out
    bool hssi_ack();

    bool do_load         (const intel::utils::cmd_handler::cmd_vector_t & cmd);
    bool do_dump         (const intel::utils::cmd_handler::cmd_vector_t & cmd);
    bool do_read         (const intel::utils::cmd_handler::cmd_vector_t & cmd);
    bool do_write        (const intel::utils::cmd_handler::cmd_vector_t & cmd);
    bool do_retimer_read (const intel::utils::cmd_handler::cmd_vector_t & cmd);
    bool do_retimer_write(const intel::utils::cmd_handler::cmd_vector_t & cmd);
    bool do_i2c_read     (const intel::utils::cmd_handler::cmd_vector_t & cmd);
    bool do_i2c_write    (const intel::utils::cmd_handler::cmd_vector_t & cmd);
    bool do_pr_read      (const intel::utils::cmd_handler::cmd_vector_t & cmd);
    bool do_pr_write     (const intel::utils::cmd_handler::cmd_vector_t & cmd);
    bool do_mdio_read    (const intel::utils::cmd_handler::cmd_vector_t & cmd);
    bool do_mdio_write   (const intel::utils::cmd_handler::cmd_vector_t & cmd);
};

} // end of namespace hssi
} // end of namespace fpga
} // end of namespace intel



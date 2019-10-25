// Copyright(c) 2017-2019, Intel Corporation
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
#include "config_app.h"
#include "hssi.h"
#include "hssi_msg.h"
#include "hssi_przone.h"
#include "dfh.h"
#include "cmd_handler.h"
#include "utils.h"
#include "fme.h"
#include "mmio_stream.h"
#include <chrono>
#include <thread>

using namespace intel::utils;

namespace intel
{
namespace fpga
{
namespace hssi
{


enum
{
    hssi_xcvr_lane_offset = 0x400
};

const std::string sysfs_path_template = "/sys/class/fpga/intel-fpga-dev.{0}/device/resource0";


static std::map<std::string, eq_register_type> str_register_type_map{
    {"FPGA_RX", eq_register_type::fpga_rx     },
    {"FPGA_TX", eq_register_type::fpga_tx     },
    {"RTMR_RX", eq_register_type::retimer_rx  },
    {"RTMR_TX", eq_register_type::retimer_tx  },
    {"MODE",    eq_register_type::hssi_mode   },
    {"MDIO",    eq_register_type::mdio        },
    {"PRZONE",  eq_register_type::przone      }
};

static std::map<eq_register_type, std::string> register_type_str_map{
    { eq_register_type::fpga_rx,     "FPGA_RX" },
    { eq_register_type::fpga_tx,     "FPGA_TX" },
    { eq_register_type::retimer_rx,  "RTMR_RX" },
    { eq_register_type::retimer_tx,  "RTMR_TX" },
    { eq_register_type::hssi_mode,   "MODE"    },
    { eq_register_type::mdio,        "MDIO"    },
    { eq_register_type::przone,      "PRZONE"  }
};


const uint32_t default_dump_size = 38;
eq_register default_dump[default_dump_size] =
{
    {eq_register_type::fpga_tx,-1,-1,0x109},
    {eq_register_type::fpga_tx,-1,-1,0x105},
    {eq_register_type::fpga_tx,-1,-1,0x106},
    {eq_register_type::fpga_tx,-1,-1,0x107},
    {eq_register_type::fpga_tx,-1,-1,0x108},
    {eq_register_type::fpga_rx,-1,-1,0x11c},
    {eq_register_type::fpga_rx,-1,-1,0x11a},
    {eq_register_type::fpga_rx,-1,-1,0x167},
    {eq_register_type::fpga_rx,-1,-1,0x160},
    {eq_register_type::fpga_rx,-1,-1,0x14d},
    {eq_register_type::fpga_rx,-1,-1,0x14f},
    {eq_register_type::fpga_rx,-1,-1,0x150},
    {eq_register_type::fpga_rx,-1,-1,0x151},
    {eq_register_type::fpga_rx,-1,-1,0x152},
    {eq_register_type::fpga_rx,-1,-1,0x153},
    {eq_register_type::fpga_rx,-1,-1,0x154},
    {eq_register_type::fpga_rx,-1,-1,0x155},
    {eq_register_type::fpga_rx,-1,-1,0x157},
    {eq_register_type::fpga_rx,-1,-1,0x158},
    {eq_register_type::fpga_rx,-1,-1,0x159},
    {eq_register_type::fpga_rx,-1,-1,0x15a},
    {eq_register_type::retimer_tx,-1,-1,0x02d},
    {eq_register_type::retimer_tx,-1,-1,0x015},
    {eq_register_type::retimer_tx,-1,-1,0x031},
    {eq_register_type::retimer_tx,-1,-1,0x01E},
    {eq_register_type::retimer_tx,-1,-1,0x011},
    {eq_register_type::retimer_tx,-1,-1,0x01F},
    {eq_register_type::retimer_rx,-1,-1,0x012},
    {eq_register_type::retimer_rx,-1,-1,0x021},
    {eq_register_type::retimer_rx,-1,-1,0x020},
    {eq_register_type::retimer_rx,-1,-1,0x040},
    {eq_register_type::retimer_rx,-1,-1,0x044},
    {eq_register_type::retimer_rx,-1,-1,0x048},
    {eq_register_type::retimer_rx,-1,-1,0x04c},
    {eq_register_type::retimer_rx,-1,-1,0x050},
    {eq_register_type::retimer_rx,-1,-1,0x054},
    {eq_register_type::retimer_rx,-1,-1,0x058},
    {eq_register_type::retimer_rx,-1,-1,0x05c},
};

const uint32_t max_i2c_device_addr    = 0xFFFF;
const uint32_t max_i2c_byte_addr      = 0xFFFF;
const uint32_t max_xcvr_lane          = 16;
const uint32_t max_xcvr_addr          = 0xFFFF;
const uint32_t max_rtmr_addr          = 0xFFFF;
const uint32_t max_rtmr_channel       = 4;

static std::vector<uint32_t> valid_rtmr_device_addrs =  { 0x30, 0x32, 0x34, 0x36 };
static std::vector<uint32_t> valid_rtmr_device_writes = { 0x31, 0x33, 0x35, 0x37 };

config_app::config_app()
: c_header_(false)
, hssi_cmd_count_(0)
, ctrl_(static_cast<uint32_t>(fme_csr::hssi_ctrl))
, stat_(static_cast<uint32_t>(fme_csr::hssi_stat))
, byte_addr_size_(1)
, header_stream_()
, input_file_("")
{
    options_.add_option<std::string>("resource",   'r', option::with_argument, "Path to syfs resource file");
    options_.add_option<uint8_t>("socket-id",      'S', option::with_argument, "Socket id encoded in BBS", 0);
    options_.add_option<uint8_t>("bus-number",     'B', option::with_argument, "Bus number of PCIe device");
    options_.add_option<uint8_t>("device",         'D', option::with_argument, "Device number of PCIe device");
    options_.add_option<uint8_t>("function",       'F', option::with_argument, "Function number of PCIe device");
    options_.add_option<bool>("c-header",          'C', option::no_argument,   "Generate a C header file to integrate into BIOS", false);
    options_.add_option<uint32_t>("byte-address-size", option::with_argument,  "Byte address width (in bytes) of I2C devices", byte_addr_size_);
    options_.add_option<bool>("help",              'h', option::no_argument,   "Show help message", false);
    options_.add_option<bool>("version",           'v', option::no_argument,   "Show version", false);

    using std::placeholders::_1;

    console_.register_handler("load",
                              std::bind(&config_app::do_load, this, _1),
                              0,
                              "[inputfile.csv] [--c-header]");
    console_.register_handler("dump",
                              std::bind(&config_app::do_dump, this, _1),
                              0,
                              "[outfile.csv] [--input-file inputfile.csv]");
    console_.register_handler("read",
                              std::bind(&config_app::do_read, this, _1),
                              2,
                              "lane(0-15) reg-address");
    console_.register_handler("write",
                              std::bind(&config_app::do_write, this, _1),
                              3,
                              "lane(0-15) reg-address value");
    console_.register_handler("rread",
                              std::bind(&config_app::do_retimer_read, this, _1),
                              2,
                              "lane(0-15) reg-address");
    console_.register_handler("rwrite",
                              std::bind(&config_app::do_retimer_write, this, _1),
                              3,
                              "lane(0-15) reg-address value");
    console_.register_handler("iread",
                              std::bind(&config_app::do_i2c_read, this, _1),
                              4,
                              "instance (0,1) device-addr byte-address byte-count");
    console_.register_handler("iwrite",
                              std::bind(&config_app::do_i2c_write, this, _1),
                              4,
                              "instance (0,1) device-addr byte-address byte1 [byte2 [byte3...]]");
    console_.register_handler("mread",
                              std::bind(&config_app::do_mdio_read, this, _1),
                              3,
                              "device-addr port_address register-address");
    console_.register_handler("mwrite",
                              std::bind(&config_app::do_mdio_write, this, _1),
                              4,
                              "device-addr port_address byte-address value");
    console_.register_handler("pread",
                              std::bind(&config_app::do_pr_read, this, _1),
                              1,
                              "address");
    console_.register_handler("pwrite",
                              std::bind(&config_app::do_pr_write, this, _1),
                              2,
                              "address value");
}


config_app::~config_app()
{

}

bool config_app::setup()
{
    options_.get_value<bool>("c-header", c_header_);
    std::string sysfs_path = "";
    int8_t socket_id = -1;
    if (options_["resource"] && options_["resource"]->is_set())
    {
        options_.get_value<std::string>("resource", sysfs_path);
    }
    else
    {
        std::string socket_str = std::to_string(socket_id);
        auto pos = sysfs_path_template.find("{0}");
        std::string sysfs_path = sysfs_path_template;
        sysfs_path =  sysfs_path.replace(pos, 3, socket_str);
    }
    options_.get_value<uint32_t>("byte-address-size", byte_addr_size_);

    if (options_["socket-id"] && options_["socket-id"]->is_set())
    {
        options_.get_value<int8_t>("socket-id", socket_id);
    }

    if (!path_exists(sysfs_path) && !c_header_)
    {
        std::cerr << "Resource path does not exist: " << sysfs_path << std::endl;
        return false;
    }

    if (c_header_)
    {
        mmio_.reset(new mmio_stream(header_stream_));
    }
    else
    {
        mmio_ = fme::open(sysfs_path, socket_id);
    }

    if (!mmio_)
    {
        std::cerr << "Error opening mmio resource" << std::endl;
        return false;
    }

    dfh_list list(mmio_);
    auto it = list.find(hssi_dfh_id, hssi_dfh_rev, dfh::priv);
    if (it != list.end())
    {
        ctrl_ = it->offset() + 0x08;
        stat_ = it->offset() + 0x10;
    }
    przone_.reset(new hssi_przone(mmio_, ctrl_, stat_));
    i2c_.reset(new i2c(std::dynamic_pointer_cast<przone_interface>(przone_), byte_addr_size_));
    mdio_.reset(new mdio(std::dynamic_pointer_cast<przone_interface>(przone_)));
    return true;

}

uint32_t config_app::run(const std::vector<std::string> & args)
{
    if (args.size() < 1)
    {
        return EXIT_FAILURE;
    }
    std::string help = "";

    if (!console_.do_cmd(args, help))
    {
        console_.writeline(help);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

void config_app::show_help()
{
    options_.show_help("hssi_config", std::cout);
    std::cout << "\nCommand help:\n";
    console_.show_help(std::cout);
}

bool config_app::do_load(const cmd_handler::cmd_vector_t & cmds)
{
    if (cmds.size() > 0)
    {
        if (path_exists(cmds[0]))
        {
            std::ifstream filestream(cmds[0]);
            load(filestream);
            return true;
        }
        else
        {
            std::cerr << "Path(" << cmds[1] << ") does not exist" << std::endl;
            return false;
        }
    }
    else
    {
        load(std::cin);
    }
    return false;

}

bool config_app::do_dump(const cmd_handler::cmd_vector_t & cmds)
{
    eq_register * registers = default_dump;
    size_t register_count = default_dump_size;
    if (input_file_ != "" && path_exists(input_file_))
    {
        std::vector<std::vector<std::string>> data;
        std::ifstream inp(input_file_);
        csv_parse(inp, data);
        std::vector<eq_register> parsed;
        parse_registers(data, parsed);
        registers = parsed.data();
        register_count = parsed.size();
    }

    if (cmds.size() > 1)
    {
        std::ofstream out(cmds[1]);
        dump(registers, register_count, out);
        return true;
    }
    else
    {
        dump(registers, register_count, std::cout);
        return true;
    }
    return false;
}

bool config_app::do_read(const cmd_handler::cmd_vector_t & cmds)
{
    int32_t lane = 0;
    uint32_t address = 0, value = 0;
    if (parse_int(cmds[0], lane) &&
        parse_int(cmds[1], address))
    {
        if (lane > (int32_t)max_xcvr_lane || address > max_xcvr_addr)
        {
            std::cerr << "invalid xcvr read parameter" << std::endl;
            return false;
        }

        if (lane < 0)
        {
            for (uint32_t i = 0; i < max_xcvr_lane; ++i)
            {
                if ( xcvr_read(i, address, value))
                {
                    std::cout << print_hex<uint32_t>(i*hssi_xcvr_lane_offset + address) << ": " <<  print_hex<uint32_t>(value) << std::endl;
                }
            }
            return true;
        }
        else
        {
            if ( xcvr_read(lane, address, value))
            {
                std::cout << print_hex<uint32_t>(value) << std::endl;
                return true;
            }
        }
    }

    return false;

}

bool config_app::do_write(const cmd_handler::cmd_vector_t & cmds)
{
    uint32_t lane = 0, address = 0, value_write = 0, value_read = 0;
    if (parse_int(cmds[0], lane) &&
        parse_int(cmds[1], address) &&
        parse_int(cmds[2], value_write))
    {

        if (lane > max_xcvr_lane || address > max_xcvr_addr)
        {
            std::cerr << "invalid xcvr write parameter" << std::endl;
            return false;
        }

        if (xcvr_write(lane, address, value_write) &&
            xcvr_read(lane, address, value_read))
        {
            std::cout << print_hex<uint32_t>(value_read) << std::endl;
            return true;
        }
    }
    return false;
}

bool config_app::do_retimer_read(const cmd_handler::cmd_vector_t & cmds)
{

    eq_register reg;
    if (parse_int(cmds[0], reg.device) &&
        parse_int(cmds[1], reg.channel_lane) &&
        parse_int(cmds[2], reg.address))
    {
        if (reg.channel_lane > static_cast<int32_t>(max_rtmr_channel))
        {
            std::cerr << "Invalid retimer channel" << std::endl;
            return false;
        }

        if (reg.address > max_rtmr_addr)
        {
            std::cerr << "Invalid retimer address" << std::endl;
        }

        if (retimer_read(reg.device, reg.channel_lane, reg.address, reg.value))
        {
            std::cout << print_hex<uint8_t>(reg.value) << std::endl;
            return true;
        }
    }

    return false;

}

bool config_app::do_i2c_read(const cmd_handler::cmd_vector_t & cmds)
{
    uint32_t instance = 0, device_addr = 0, byte_addr = 0, size = 0;
    if (parse_int(cmds[0], instance) &&
        parse_int(cmds[1], device_addr) &&
        parse_int(cmds[2], byte_addr) &&
        parse_int(cmds[3], size) &&
        size > 0)
    {
        if (instance > 1 || device_addr > max_i2c_device_addr || byte_addr > max_i2c_byte_addr )
        {
            std::cerr << "invalid i2c parameters" << std::endl;
            return false;
        }

        uint8_t bytes[size];
        if (i2c_->read(instance, device_addr, byte_addr, bytes, size))
        {
            for (const auto & byte : bytes)
            {
                std::cout << print_hex<uint8_t>(byte) << " ";
            }
            std::cout << std::endl;
            return true;
        }
    }
    return false;
}

bool config_app::do_i2c_write(const cmd_handler::cmd_vector_t & cmds)
{
    uint32_t instance = 0, device_addr = 0, byte_addr = 0;
    if (parse_int(cmds[0], instance) &&
        parse_int(cmds[1], device_addr) &&
        parse_int(cmds[2], byte_addr))
    {
        if (instance > 1 || device_addr > max_i2c_device_addr || byte_addr > max_i2c_byte_addr )
        {
            std::cerr << "invalid i2c parameters" << std::endl;
            return false;
        }

        std::vector<uint8_t> bytes(cmds.size()-3);
        int i = 0;
        std::for_each(cmds.begin()+3, cmds.end(),
                [&bytes, &i](const std::string &s)
                {
                    if (!parse_int(s, bytes[i++]))
                    {
                        std::cerr  << "Could not parse input: " << s << std::endl;
                    }
                });

        if (i2c_->write(instance, device_addr, byte_addr, bytes.data(), bytes.size()))
        {
            for (const auto & byte : bytes)
            {
                std::cout << print_hex<uint8_t>(byte) << " ";
            }
            std::cout << std::endl;
            return true;
        }
    }
    return false;
}

bool config_app::do_retimer_write(const cmd_handler::cmd_vector_t & cmds)
{
    eq_register reg;
    if (parse_int(cmds[0], reg.device) &&
        parse_int(cmds[1], reg.channel_lane) &&
        parse_int(cmds[2], reg.address) &&
        parse_int(cmds[3], reg.value))
    {
        if (reg.address > max_rtmr_addr)
        {
            std::cerr << "Invalid retimer address" << std::endl;
        }

        if (retimer_write(reg.device, reg.channel_lane, reg.address, reg.value) &&
            retimer_read(reg.device, reg.channel_lane, reg.address, reg.value))
        {
            std::cout << print_hex<uint32_t>(reg.value) << std::endl;
            return true;
        }
    }
    return false;

}

bool config_app::do_mdio_write(const cmd_handler::cmd_vector_t & cmds)
{
    uint8_t device_addr = 0, port_addr = 0;
    uint16_t reg_addr = 0;
    uint32_t value = 0;
    if (parse_int(cmds[0], device_addr) &&
        parse_int(cmds[1], port_addr) &&
        parse_int(cmds[2], reg_addr) &&
        parse_int(cmds[3], value))
    {
        if (mdio_->write(device_addr, port_addr, reg_addr, value))
        {
            std::cout << print_hex<uint32_t>(value) << std::endl;
            return true;
        }
    }

    return false;
}

bool config_app::do_mdio_read(const cmd_handler::cmd_vector_t & cmds)
{
    uint8_t device_addr = 0, port_addr = 0;
    uint16_t reg_addr = 0;

    if (parse_int(cmds[0], device_addr) &&
        parse_int(cmds[1], port_addr) &&
        parse_int(cmds[2], reg_addr) )
    {
        uint32_t value;
        if (mdio_->read(device_addr, port_addr, reg_addr, value))
        {
            std::cout << print_hex<uint32_t>(value) << std::endl;
            return true;
        }
    }

    return false;
}

bool config_app::do_pr_read(const cmd_handler::cmd_vector_t & cmds)
{
    uint32_t reg_addr = 0;
    if (parse_int(cmds[0], reg_addr))
    {
        uint32_t value;
        if (przone_->read(reg_addr, value))
        {
            std::cout << print_hex<uint32_t>(reg_addr) << ":" << print_hex<uint32_t>(value) << std::endl;
            return true;
        }
    }

    return false;
}

bool config_app::do_pr_write(const cmd_handler::cmd_vector_t & cmds)
{
    uint32_t reg_addr = 0;
    uint32_t value = 0;
    if (parse_int(cmds[0], reg_addr) &&
        parse_int(cmds[1], value))
    {
        if (przone_->write(reg_addr, value))
        {
            std::cout << print_hex<uint32_t>(reg_addr) << ":" << print_hex<uint32_t>(value) << std::endl;
            return true;
        }
    }

    return false;
}


void config_app::load(std::istream & stream)
{

    std::vector<std::vector<std::string>> data;
    csv_parse(stream, data);

    std::vector<eq_register> registers;
    parse_registers(data, registers);
    load(registers.data(), registers.size());
    if (registers.size() == 0)
    {
        std::cerr << "No register data parsed. Nothing to do" << std::endl;
        return;
    }

    if (c_header_)
    {
        std::string cmds = header_stream_.str();

        std::cout << "#ifndef __hssi_cmd_table_h\n";
        std::cout << "#define __hssi_cmd_table_h\n";
        std::cout << "#include <stdint.h>\n";
        std::cout << "uint64_t eq_registers[] = {\n";
        std::cout << cmds;
        std::cout << "\t-1UL\n};\n";
        std::cout << "#endif\n";
    }
}

size_t config_app::parse_registers(const std::vector<std::vector<std::string>> & data,
                                   std::vector<eq_register> &registers)
{
    size_t count = 0;


    registers.resize(data.size());
    std::transform(data.begin(), data.end(), registers.begin(),
            [&count](const std::vector<std::string> & line) -> eq_register
            {
                eq_register reg;
                uint8_t reg_type = 0;
                if (std::isdigit(line[0][0]))
                {
                    if (parse_int(line[0], reg_type))
                    {
                        reg.type = static_cast<eq_register_type>(reg_type);
                    }
                }
                else
                {
                    auto it = str_register_type_map.find(line[0]);
                    if (it != str_register_type_map.end())
                    {
                        reg.type = static_cast<eq_register_type>(it->second);
                    }
                }
                if (line.size() > 3 &&
                    parse_int(line[1], reg.channel_lane) &&
                    parse_int(line[2], reg.device) &&
                    parse_int(line[3], reg.address))
                {
                    if (line.size() > 4)
                    {
                        if (parse_int(line[4], reg.value))
                        {
                            ++count;
                        }
                    }
                    else if (line.size() == 4)
                    {
                        ++count;
                    }
                    return reg;
                }

                return eq_register();
            });
    return count;
}

size_t config_app::load(eq_register registers[], size_t size)
{
    size_t loaded = 0;
    for (size_t i = 0; i < size; ++i)
    {
        const eq_register & reg = registers[i];
        switch(reg.type)
        {
            case eq_register_type::fpga_rx:
            case eq_register_type::fpga_tx:
                xcvr_write(reg.channel_lane, reg.address, reg.value);
                break;

            case eq_register_type::retimer_rx:
            case eq_register_type::retimer_tx:
                if (!c_header_)
                {
                    retimer_write(reg.device, reg.channel_lane, reg.address, reg.value);
                }
                break;
            case eq_register_type::hssi_mode:
                if (c_header_)
                {
                    header_stream_ << "// GOTO MODE:  "
                                   << print_hex<uint32_t>(reg.value) << std::endl;
                }
                hssi_soft_cmd(nios_cmd::change_hssi_mode, std::vector<uint32_t>{ reg.value  });
                hssi_soft_cmd(nios_cmd::hssi_init, std::vector<uint32_t>{ reg.value  });
                break;
            case eq_register_type::mdio:
                if (!c_header_)
                {
                    mdio_->write(reg.channel_lane, reg.device, reg.address, reg.value);
                    std::cerr << "// MDIO Write Device: " << print_hex<uint8_t>(reg.channel_lane) << " "
                     << "port:" << " " << print_hex<uint8_t>(reg.device) << " "
                     << "reg:" << " " << print_hex<uint16_t>(reg.address) << " "
                     << print_hex<uint32_t>(reg.value) << std::endl;
                }
                break;
            case eq_register_type::przone:
                if (!c_header_)
                {
                    przone_->write(reg.address, reg.value);
                }
                break;
            default: break;
        }
    }
    return loaded;
}

bool config_app::hssi_soft_cmd(uint32_t nios_func, std::vector<uint32_t> args)
{
    uint32_t junk;
    return hssi_soft_cmd(nios_func, args, junk);
}

bool config_app::hssi_soft_cmd(uint32_t nios_func, std::vector<uint32_t> args, uint32_t & value_out)
{
    if (args.size() > 4)
    {
        return false;
    }

    hssi_ctrl msg;
    for (size_t i = 0; i < args.size(); ++i)
    {
        msg.clear();
        msg.set_command(hssi_cmd::sw_write);
        msg.set_address(i+2);
        msg.set_data(args[i]);
        mmio_->write_mmio64(ctrl_, msg.data());
        if (!hssi_ack())
        {
            return false;
        }
    }

    msg.set_command(hssi_cmd::sw_write);
    msg.set_address(1);
    msg.set_data(nios_func);
    mmio_->write_mmio64(ctrl_, msg.data());
    if (!hssi_ack())
    {
        return false;
    }

    switch(nios_func)
    {
        case nios_cmd::tx_eq_read:
        case nios_cmd::hssi_init:
        case nios_cmd::hssi_init_done:
        case nios_cmd::fatal_err:
        case nios_cmd::get_hssi_enable:
        case nios_cmd::get_hssi_mode:
            msg.set_command(hssi_cmd::sw_read);
            msg.set_address(6);
            msg.set_data(nios_func);
            mmio_->write_mmio64(ctrl_, msg.data());
            if (!hssi_ack())
            {
                return false;
            }
            uint64_t value64;

            if (!mmio_->read_mmio64(stat_, value64))
            {
                return false;
            }
            value_out = static_cast<uint32_t>(value64);
            break;
        default:
            break;
    }

    return true;
}

size_t config_app::dump(eq_register registers[], size_t size, std::ostream & stream)
{
    std::vector<eq_register> dumped;

    // add HSSI mode to dump vector first
    uint32_t hssi_mode = 0;
    if (!hssi_soft_cmd(nios_cmd::get_hssi_mode, {}, hssi_mode))
    {
        std::cerr << "Error executing soft cmd: get_hssi_mode" << std::endl;
        return false;
    }

    // now add registers to dump vector
    for (size_t i = 0; i < size; ++i)
    {
        eq_register & reg = registers[i];
        switch(reg.type)
        {
            case eq_register_type::fpga_rx:
            case eq_register_type::fpga_tx:
                if (reg.channel_lane == -1)
                {
                    for(int32_t i = 0; i < static_cast<int32_t>(max_xcvr_lane); ++i)
                    {
                        eq_register cpy = reg;
                        cpy.channel_lane = i;
                        if (xcvr_read(cpy.channel_lane, cpy.address, cpy.value))
                        {
                            dumped.push_back(cpy);
                        }
                    }
                }
                else if (xcvr_read(reg.channel_lane, reg.address, reg.value))
                {
                    dumped.push_back(reg);
                }
                break;

            case eq_register_type::retimer_rx:
            case eq_register_type::retimer_tx:
                if (reg.channel_lane == -1 || reg.device == -1)
                {
                    for (uint32_t i : valid_rtmr_device_addrs)
                    {
                        for (int32_t j = 0; j < static_cast<int32_t>(max_rtmr_channel); ++j)
                        {
                            eq_register cpy = reg;
                            cpy.device = i;
                            cpy.channel_lane = j;
                            if(retimer_read(cpy.device, cpy.channel_lane, cpy.address, cpy.value))
                            {
                                cpy.value &= 0xFF;
                                dumped.push_back(cpy);
                            }
                        }
                    }
                }
                else
                {
                    if (retimer_read(reg.device, reg.channel_lane, reg.address, reg.value))
                    {
                        dumped.push_back(reg);
                    }
                }
                break;
            case eq_register_type::hssi_mode:
                if (c_header_)
                {
                    header_stream_ << "// GOTO MODE:  "
                                   << print_hex<uint32_t>(reg.value) << std::endl;
                }
                else if (hssi_soft_cmd(nios_cmd::get_hssi_mode, {}, hssi_mode))
                {
                      dumped.push_back({eq_register_type::hssi_mode, -1, -1, 0, hssi_mode});
                }
                break;
            case eq_register_type::mdio:
                if (!c_header_)
                {
                    eq_register cpy = reg;
                    if (mdio_->read(cpy.channel_lane, cpy.device, cpy.address, cpy.value))
                    {
                       dumped.push_back(cpy);
                    }
                }
                break;
            case eq_register_type::przone:
                if (!c_header_)
                {
                    eq_register cpy = reg;
                    if (przone_->read(cpy.address, cpy.value))
                    {
                       dumped.push_back(cpy);
                    }
                }
                break;

            default:
                break;
        }
    }

    // for all registers in dump vector, write them out to the stream
    for (const auto & reg : dumped)
    {
        if (register_type_str_map.find(reg.type) == register_type_str_map.end())
        {
            std::cerr << "UNKNOWN TYPE!";
        }
        else
        {
            stream << register_type_str_map[reg.type];
        }

        stream << "," << reg.channel_lane
               << "," << print_hex<uint16_t>(reg.device)
               << "," << print_hex<uint32_t>(reg.address)
               << "," << print_hex<uint8_t>(reg.value) << std::endl;
    }
    return dumped.size();
}


bool config_app::xcvr_pll_status_read(uint32_t info_sel, uint32_t &value)
{
    hssi_ctrl msg;

    // 3. Read data from local bus to aux bus
    // Write recfg_cmd_rddata to aux_bus::local_cmd
    // recfg_cmd_rddata (from recfg_cmd_addr), output goes into local_dout
    msg.clear();
    msg.set_address(aux_bus::local_cmd);
    if (info_sel == 0)
        msg.set_bus_command(bus_cmd::local_read, local_bus::pll_rst_control);
    else
        msg.set_bus_command(bus_cmd::local_read, local_bus::pll_locked_status);
    msg.set_command(hssi_cmd::aux_write);
    mmio_->write_mmio64(ctrl_, msg.data());

    if (!hssi_ack())
    {
        return false;
    }

    // 4. Read data from aux bus to hssi_stat
    // Read local_dout
    msg.clear();
    msg.set_address(aux_bus::local_dout);
    msg.set_command(hssi_cmd::aux_read);
    mmio_->write_mmio64(ctrl_, msg.data());

    if (!hssi_ack())
    {
        return false;
    }

    uint64_t hssi_value;
    if (!mmio_->read_mmio64(stat_, hssi_value))
    {
        return false;
    }

    value = static_cast<uint32_t>(hssi_value & 0x00000000FFFFFFFF);
    return true;
}


bool config_app::xcvr_read(uint32_t lane, uint32_t reg_addr, uint32_t &value)
{
    uint32_t reconfig_addr = reg_addr + hssi_xcvr_lane_offset*lane;
    // HSSI_CTRL cmd(63-48), addr(47-32), data(31-0)

    // 1. Provide ID information of local register to AUX bus handshake registers
    // Write read_cmd, channel, reconfig_addr to aux_bus::local_din
    // read_cmd, channel, reconfig_addr -> local_din
    hssi_ctrl msg;

    msg.clear();
    msg.set_address(aux_bus::local_din);
    msg.set_bus_command(bus_cmd::rcfg_read, reconfig_addr);
    msg.set_command(hssi_cmd::aux_write);
    mmio_->write_mmio64(ctrl_, msg.data());

    if (!hssi_ack())
    {
        return false;
    }


    // 2. Send request to AUX bus handshake, write ID information from step 1 into local bus
    // Write write_cmd, recfg_cmd_addr to aux_bus::local_cmd
    // write_cmd from local_din (read_cmd, channel, reconf_addr) -> recfg_cmd_addr
    msg.clear();
    msg.set_address(aux_bus::local_cmd);
    msg.set_bus_command(bus_cmd::local_write, local_bus::recfg_cmd_addr);
    msg.set_command(hssi_cmd::aux_write);
    mmio_->write_mmio64(ctrl_, msg.data());

    if (!hssi_ack())
    {
        return false;
    }


    // 3. Read data from local bus to aux bus
    // Write recfg_cmd_rddata to aux_bus::local_cmd
    // recfg_cmd_rddata (from recfg_cmd_addr), output goes into local_dout
    msg.clear();
    msg.set_address(aux_bus::local_cmd);
    msg.set_bus_command(bus_cmd::local_read, local_bus::recfg_cmd_rddata);
    msg.set_command(hssi_cmd::aux_write);
    mmio_->write_mmio64(ctrl_, msg.data());

    if (!hssi_ack())
    {
        return false;
    }


    // 4. Read data from aux bus to hssi_stat
    // Read local_dout
    msg.clear();
    msg.set_address(aux_bus::local_dout);
    msg.set_command(hssi_cmd::aux_read);
    mmio_->write_mmio64(ctrl_, msg.data());

    if (!hssi_ack())
    {
        return false;
    }


    uint64_t hssi_value;
    if (!mmio_->read_mmio64(stat_, hssi_value))
    {
        return false;
    }

    value = static_cast<uint32_t>(hssi_value & 0x00000000FFFFFFFF);
    return true;
}


bool config_app::xcvr_write(uint32_t lane, uint32_t reg_addr, uint32_t value)
{
    if (c_header_)
    {
        header_stream_ << "\t// XCVR: Lane " << +lane
                     << ", Address " << print_hex<uint32_t>(reg_addr) << " "
                     << ", Value " << print_hex<uint32_t>(value) << std::endl;
    }
    // 1. Provide the data to be written into reconf register to the AUX bus
    // Write value to local_din
    uint32_t reconfig_addr = reg_addr + hssi_xcvr_lane_offset*lane;
    hssi_ctrl msg;

    msg.clear();
    msg.set_address(aux_bus::local_din);
    msg.set_data(static_cast<uint32_t>(value));
    msg.set_command(hssi_cmd::aux_write);
    mmio_->write_mmio64(ctrl_, msg.data());

    if (!hssi_ack())
    {
        return false;
    }


    // 2. Send request to AUX bus handshake registers, to write data into RCFG bus
    // Write from local_din to local_dout
    msg.clear();
    msg.set_address(aux_bus::local_cmd);
    msg.set_bus_command(bus_cmd::local_write, aux_bus::local_dout);
    msg.set_command(hssi_cmd::aux_write);
    mmio_->write_mmio64(ctrl_, msg.data());

    if (!hssi_ack())
    {
        return false;
    }


    // 3. Provide ID of reconf register to AUX bus handshake register
    // Write local_dout to channel+reconfig_addr
    msg.clear();
    msg.set_address(aux_bus::local_din);
    msg.set_bus_command(bus_cmd::local_write, reconfig_addr);
    msg.set_command(hssi_cmd::aux_write);
    mmio_->write_mmio64(ctrl_, msg.data());

    if (!hssi_ack())
    {
        return false;
    }


    // 4. Send the request to the AUX bus handshake registers, write ID information from step 3
    msg.clear();
    msg.set_address(aux_bus::local_cmd);
    msg.set_bus_command(bus_cmd::local_write, aux_bus::local_din);
    msg.set_command(hssi_cmd::aux_write);
    mmio_->write_mmio64(ctrl_, msg.data());

    if (!hssi_ack())
    {
        return false;
    }

    return true;
}

bool config_app::retimer_write(uint32_t device_addr, uint8_t channel, uint32_t address, uint32_t value)
{
    if (std::find(valid_rtmr_device_addrs.begin(), valid_rtmr_device_addrs.end(),
                  device_addr) == valid_rtmr_device_addrs.end())
    {
        std::cerr << "Invalid retimer address: " << print_hex<uint32_t>(device_addr) << std::endl;
        return false;
    }

    if (c_header_)
    {
        header_stream_ << "// RTMR: " << print_hex<uint32_t>(device_addr) << " "
                     << +channel << " " << print_hex<uint32_t>(address) << " "
                     << print_hex<uint32_t>(value) << std::endl;
    }
    uint8_t channel_byte = channel+4;
    uint8_t channel_select_byte[1]= {channel_byte};
    i2c_->write(i2c_instance_retimer, device_addr, 0xFF, channel_select_byte, 1UL);
    uint8_t * ptr = reinterpret_cast<uint8_t*>(&value);
    i2c_->write(i2c_instance_retimer, device_addr, address, ptr, sizeof(uint32_t));
    return true;
}

bool config_app::retimer_read(uint32_t device_addr, uint8_t channel, uint32_t address, uint32_t & value)
{
    if (std::find(valid_rtmr_device_addrs.begin(), valid_rtmr_device_addrs.end(),
                  device_addr) == valid_rtmr_device_addrs.end())
    {
        std::cerr << "Invalid retimer address: " << print_hex<uint32_t>(device_addr) << std::endl;
        return false;
    }

    uint8_t channel_byte = channel+4;
    uint8_t channel_select_byte[1]= {channel_byte};
    i2c_->write(i2c_instance_retimer, device_addr, 0xFF, channel_select_byte, 1UL);
    uint8_t * ptr = reinterpret_cast<uint8_t*>(&value);
    i2c_->read(i2c_instance_retimer, device_addr, address, ptr, sizeof(uint32_t));
    return true;
}

bool config_app::wait_for_ack(config_app::ack_t response, uint32_t timeout_usec, uint32_t * duration)
{
    using hrc = std::chrono::high_resolution_clock;
    using std::chrono::microseconds;
    auto begin  = hrc::now();
    static const uint32_t ack_bit = 32;

    uint64_t value = response == ack_t::ack ? 0 : 0xFFFF;
    // write a little lambda to check the value basked on response type we are
    // waiting on
    auto check_ack = [response](uint64_t v) -> bool
    {
        return response == ack_t::ack ?  (v & (1UL << ack_bit)) : (~v & (1UL << ack_bit));
    };

    bool timedout = false;
    while (!timedout)
    {
        auto delta = hrc::now() - begin;

        if (mmio_->read_mmio64(stat_, value) && check_ack(value))
        {
            if (duration)
            {
                *duration = std::chrono::duration_cast<microseconds>(delta).count();
            }

            return true;
        }

        std::this_thread::sleep_for(microseconds(10));
        timedout = delta > microseconds(timeout_usec);
    }
    return false;



}

bool config_app::hssi_ack()
{
    if (c_header_)
    {
        header_stream_ << ",\n";
        ++hssi_cmd_count_;
        return true;
    }

    if (!wait_for_ack(ack_t::ack))
    {
        return false;
    }
    mmio_->write_mmio64(ctrl_, 0UL);
    if (!wait_for_ack(ack_t::nack))
    {
        return false;
    }
    return true;

}

} // end of namespace hssi
} // end of namespace fpga
} // end of namespace intel

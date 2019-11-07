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

#include "loopback_app.h"
#include "accelerator_przone.h"
#include "cmd_handler.h"
#include "utils.h"
#include <chrono>
#include <csignal>

using namespace intel::utils;

namespace
{
    volatile std::sig_atomic_t g_sigcount = 0;
}

namespace intel
{
namespace fpga
{
namespace hssi
{

const uint32_t min_packet_size = 46;
const uint32_t max_packet_size = 1500;

loopback_app::loopback_app()
: timeout_(60.0)
, delay_(0.100)
, ports_(0)
{
    options_.add_option<uint8_t>("socket-id",      'S', option::with_argument, "Socket id encoded in BBS");
    options_.add_option<uint8_t>("bus",            'B', option::with_argument, "Bus number of PCIe device");
    options_.add_option<uint8_t>("device",         'D', option::with_argument, "Device number of PCIe device");
    options_.add_option<uint8_t>("function",       'F', option::with_argument, "Function number of PCIe device");
    options_.add_option<std::string>("guid",       'G', option::with_argument, "GUID of AFC to open");
    options_.add_option<std::string>("mode",       'm', option::with_argument, "Mode of loopback - auto, e10, e40, e100", "auto");
    options_.add_option<double>("timeout",         't', option::with_argument, "Timeout (sec)", timeout_);
    options_.add_option<double>("delay",           'y', option::with_argument, "Delay between status reports - default is 100 msec", delay_);
    options_.add_option<uint32_t>("packet-count",  'c', option::with_argument, "Number of packets to send");
    options_.add_option<uint32_t>("packet-delay",  'd', option::with_argument, "Delay between packets (in cycles)");
    options_.add_option<uint32_t>("packet-length", 'l', option::with_argument, "Length of packets (in bytes)");
    options_.add_option<bool>    ("random-length", 'r', option::no_argument, "Choose random packet length", false);
    options_.add_option<bool>("help"                  , option::no_argument, "Show help message", false);
    options_.add_option<bool>("version",           'v', option::no_argument, "Show version", false);

    using std::placeholders::_1;

    console_.register_handler("send",
                              std::bind(&loopback_app::do_loopback, this, _1),
                              2,
                              "<source port> [<destination port>] [-c|--packet-count <packet count>] "
                              "[-l|--packet-length <packet length>] "
                              "# Start loopback from soure to destination. If no destination is given, internal loopback test is performed");
    console_.register_handler("stop",
                              std::bind(&loopback_app::do_stop, this, _1),
                              0,
                              "stop any running test");
    console_.register_handler("status",
                              std::bind(&loopback_app::do_status, this, _1),
                              0,
                              "[clear] # show/clear port stats");
    console_.register_handler("readmacs",
                              std::bind(&loopback_app::do_readmacs, this, _1),
                              0,
                              "read and show mac addresses in eeprom");
}


loopback_app::~loopback_app()
{

}

void loopback_app::show_help()
{
    options_.show_help("hssi_loopback", std::cout);
    std::cout << "\nCommand help:\n";
    console_.show_help(std::cout);
}

uint32_t loopback_app::run(loopback::ptr_t lpbk, const std::vector<std::string> & args)
{
    lpbk_ = lpbk;
    if (!lpbk_->initialize())
    {
        std::cerr << "Error initializing accelerator" << std::endl;
        return EXIT_FAILURE;
    }

    if (args.size() == 0)
    {
        std::cerr << "No commands specified" << std::endl;
        return EXIT_FAILURE;
    }

    if (console_.have_cmd(args[0]))
    {
        std::string help = "";
        if (!console_.do_cmd(args, help))
        {
            std::cerr << help << std::endl;
            return EXIT_FAILURE;
        }
    }
    else if (!do_loopback(args))
    {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}


bool loopback_app::do_loopback(const cmd_handler::cmd_vector_t & cmds)
{
    if (cmds.size() == 0)
    {
        return EXIT_FAILURE;
    }
    using std::chrono::microseconds;
    using std::chrono::seconds;

    bool cont = true;

    auto packet_count  = options_.find("packet-count");
    auto packet_length = options_.find("packet-length");
    auto packet_delay  = options_.find("packet-delay");
    auto random_length = options_.find("random-length");

    uint32_t packet_size = min_packet_size;
    if (packet_count && packet_count->is_set())
    {
        cont = false;
        lpbk_->packet_count(packet_count->value<uint32_t>());
    }

    if (packet_length && packet_length->is_set())
    {
        packet_size = packet_length->value<uint32_t>();
        if (packet_size < min_packet_size)
        {
            packet_size = min_packet_size;
        }
        else if (packet_size > max_packet_size)
        {
            packet_size = max_packet_size;
        }
        lpbk_->packet_length(packet_length->value<uint32_t>());
    }
    else if (random_length && random_length->is_set())
    {
        lpbk_->random_length(true);
    }

    if (packet_delay && packet_delay->is_set())
    {
        lpbk_->packet_delay(packet_delay->value<uint32_t>());
    }

    lpbk_->continuous(cont);

    bool external = false;
    if (cmds[0] == "all" || cmds[0] == "internal")
    {
        // do external if "all" is selected, otherwise go internal
        external = cmds[0] == "all";
        // TODO: figure out port pairs from lpbk_ instance (other than num_ports)
        // For example:
        // e10 uses four ports wired such that external loopback goes
        // from one port to itself (0->0, 1->1, 2->2, 3->3)
        // e40 uses two ports wired such that external loopback goes
        // from one port to the other (0->1, 1->0)
        auto num_ports = lpbk_->num_ports();
        switch(num_ports)
        {
            case 4:
                ports_.push_back(std::make_pair(0,0));
                ports_.push_back(std::make_pair(1,1));
                ports_.push_back(std::make_pair(2,2));
                ports_.push_back(std::make_pair(3,3));
                break;
            case 2:
                ports_.push_back(std::make_pair(0,1));
                ports_.push_back(std::make_pair(1,0));
                break;
            case 1:
                ports_.push_back(std::make_pair(0,0));
                break;
        }
    }
    else
    {
        ports_.resize(1);
        switch(cmds.size())
        {
            case 2 :
                if (!parse_int(cmds[1], ports_[0].second))
                {
                    return false;
                }
                external = true;
                if (!parse_int(cmds[0], ports_[0].first))
                {
                    return false;
                }
                break;
            case 1 :
                if (!parse_int(cmds[0], ports_[0].first))
                {
                    return false;
                }
                break;
            case 0 :
                return false;
        }
    }

    lpbk_->clear_status();
    for(auto & p : ports_)
    {
        if (external)
        {
            lpbk_->external_loopback(p.first, p.second);
        }
        else
        {
            p.second = p.first;
            lpbk_->internal_loopback(p.first);
        }
    }

    bool success = wait_for_loopback(external);

    if (!success)
    {
        console_.writeline("ERROR encountered in loopback");
    }

    return success;
}


bool loopback_app::wait_for_loopback(bool external)
{
    using std::chrono::microseconds;
    using std::chrono::seconds;

    using hrc = std::chrono::high_resolution_clock;

    std::signal(SIGINT, [](int signal){
        UNUSED_PARAM(signal);
        ++g_sigcount;
    });


    options_.get_value<double>("timeout", timeout_);
    options_.get_value<double>("delay", delay_);
    bool stop = false, timedout = false;
    generators_.resize(ports_.size());
    monitors_.resize(ports_.size());


    auto begin = hrc::now();

    auto update_reports = [this]()
    {
        std::transform(ports_.begin(), ports_.end(), generators_.begin(),
                [this](const loopback::pair_t & p)
                {
                    return lpbk_->gen_report(p.first);
                });
        std::transform(ports_.begin(), ports_.end(), monitors_.begin(),
                [this](const loopback::pair_t & p)
                {
                    return lpbk_->gen_report(p.second);
                });
    };

    auto all_done = [this](bool check_stats = false)
    {
        for (std::vector<mac_report>::size_type i = 0; i < generators_.size(); ++i)
        {
            if (!generators_[i].gen_complete)
            {
                return false;
            }
            if (!monitors_[i].mon_complete)
            {
                return false;
            }
            if (check_stats)
            {
                if (generators_[i].cntr_tx_stat != monitors_[i].cntr_rx_stat)
                {
                    return false;
                }
                if (monitors_[i].cntr_rx_pause  > 0   ||
                    monitors_[i].cntr_rx_frag   > 0   ||
                    monitors_[i].cntr_rx_crcerr > 0   ||
                    monitors_[i].mon_dest_error       ||
                    monitors_[i].mon_src_error        ||
                    monitors_[i].mon_pkt_length_error )
                    return false;

            }
        }

        return true;
    };

    auto print_stats = [this, external](double delta)
    {
        UNUSED_PARAM(delta);
        for (std::vector<mac_report>::size_type i = 0; i < generators_.size(); ++i)
        {
            std::cerr << "port " << generators_[i].port;
            if (external)
            {
                std::cerr << " -> " << monitors_[i].port;
            }
            std::cerr <<  ", ";
            std::cerr <<  "tx: " << generators_[i].cntr_tx_stat << ", ";
            std::cerr <<  "rx: " << monitors_[i].cntr_rx_stat << "\n";
            // TODO: remove or take into account the packet delay in the hardware
            // std::cerr << static_cast<double>(monitors_[i].cntr_rx_stat*packet_size)/delta * 8.0 * 1.0E-9 << " Gbs \n";
        }
    };


    update_reports();

    while(!stop)
    {
        auto now = hrc::now();
        std::chrono::duration<double> delta = (now - begin);
        timedout = now - begin > std::chrono::duration<double>(timeout_);
        if (timedout || g_sigcount == 1)
        {
            do_stop({});
        }

        print_stats(delta.count());
        std::this_thread::sleep_for(std::chrono::duration<double>(delay_));
        update_reports();
        stop = all_done() || g_sigcount > 1;
    }
    update_reports();
    std::vector<mac_report> reports;
    for(uint32_t i = 0; i < lpbk_->num_ports(); ++i)
    {
        reports.push_back(lpbk_->gen_report(i));
    }
    std::this_thread::sleep_for(microseconds(100));
    console_.writeline(reports);
    return all_done(true);

}

bool loopback_app::do_stop(const cmd_handler::cmd_vector_t & cmds)
{
    UNUSED_PARAM(cmds);
    using std::chrono::microseconds;
    uint32_t delay_us = 100;
    for (auto p : ports_)
    {
        lpbk_->stop(p.first, loopback::packet_flow::generator);
    }

    std::this_thread::sleep_for(microseconds(delay_us));

    for (auto p : ports_)
    {
        lpbk_->stop(p.second, loopback::packet_flow::monitor);
    }
    return true;
}

bool loopback_app::do_readmacs(const cmd_handler::cmd_vector_t & cmds)
{
    int begin = 0, end = lpbk_->num_ports();
    if (cmds.size() > 0)
    {
        if (parse_int(cmds[0], begin))
        {
            end = begin+1;
        }
        else
        {
            return false;
        }
    }
    auto macs = lpbk_->get_mac_addresses();
    for (int i = begin; i < end; ++i)
    {
        const mac_address_t & mac = macs[i];
        console_.writeline("PORT ", i,  ": ",
                           "hi, "  , print_hex<uint32_t>(mac.hi),
                           " lo, " , print_hex<uint32_t>(mac.lo));
    }

    return true;

}

bool loopback_app::do_status(const cmd_handler::cmd_vector_t & cmds)
{
    if (cmds.size() > 0 && cmds[0] == "clear")
    {
        lpbk_->clear_status();
    }
    std::vector<mac_report> reports;
    for (uint32_t i=0; i < lpbk_->num_ports(); ++i)
    {
        reports.push_back(lpbk_->gen_report(i));
    }
    console_.writeline(reports);
    return true;

}

std::ostream & operator<<(std::ostream & os, const std::vector<intel::fpga::hssi::mac_report> & reports)
{
    using intel::fpga::hssi::mac_report;
    auto status = [](bool is_complete)
                  {
                      return is_complete ? "stopped" : "running";
                  };

    os <<  "PORT         : ";
    int width = 16;

    for (const auto & r : reports) os << std::setw(width) << r.port << "|";
    os << std::endl;

    os <<  "________________" << std::setfill('_');
    for (std::size_t i = 0; i < reports.size(); ++i) os << std::setw(width) << "__";
    os << std::setfill(' ') << std::endl;

    os <<  "TX STAT      : ";
    for (const auto & r : reports) os << std::setw(width) << r.cntr_tx_stat << "|";
    os << std::endl;

    os <<  "RX STAT      : ";
    for (const auto & r : reports) os << std::setw(width) << r.cntr_rx_stat << "|";
    os << std::endl;

    os <<  "RX PAUSE     : ";
    for (const auto & r : reports) os << std::setw(width) << r.cntr_rx_pause << "|";
    os << std::endl;

    os <<  "RX FRAG      : ";
    for (const auto & r : reports) os << std::setw(width) << r.cntr_rx_frag << "|";
    os << std::endl;

    os <<  "RX CRC ERR   : ";
    for (const auto & r : reports) os <<  std::setw(width) << std::boolalpha << r.cntr_rx_crcerr << "|";
    os << std::endl;

    os <<  "DST ADDR ERR : ";
    for (const auto & r : reports) os <<  std::setw(width) << std::boolalpha << r.mon_dest_error << "|";
    os << std::endl;

    os <<  "SRC ADDR ERR : ";
    for (const auto & r : reports) os <<  std::setw(width) << std::boolalpha << r.mon_src_error << "|";
    os << std::endl;

    os <<  "PKT LEN ERR  : ";
    for (const auto & r : reports) os << std::setw(width) << std::boolalpha << r.mon_pkt_length_error << "|";
    os << std::endl;

    os <<  "GEN STATUS   : ";
    for (const auto & r : reports) os <<  std::setw(width) << status(r.gen_complete) << "|";
    os << std::endl;

    os <<  "MON STATUS   : ";
    for (const auto & r : reports) os <<  std::setw(width) << status(r.mon_complete) << "|";
    os << std::endl;
    return os;

}

} // end of namespace hssi
} // end of namespace fpga
} // end of namespace intel

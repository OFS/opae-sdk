#! /usr/bin/env python3
# Copyright(c) 2021-2022, Intel Corporation
#
# Redistribution  and  use  in source  and  binary  forms,  with  or  without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of  source code  must retain the  above copyright notice,
#  this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright notice,
#  this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
# * Neither the name  of Intel Corporation  nor the names of its contributors
#   may be used to  endorse or promote  products derived  from this  software
#   without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO,  THE
# IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT  SHALL THE COPYRIGHT OWNER  OR CONTRIBUTORS BE
# LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
# CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT LIMITED  TO,  PROCUREMENT  OF
# SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  DATA, OR PROFITS;  OR BUSINESS
# INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY  OF LIABILITY,  WHETHER IN
# CONTRACT,  STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE  OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

import argparse
import sys
from ethernet.hssicommon import *

# Sleep 50 milliseconds after clearing stats.
HSSI_STATS_CLEAR_SLEEP_TIME = 50/1000


class FPGAHSSISTATS(HSSICOMMON):
    hssi_eth_stats = (('tx_packets', 0),
                      ('rx_packets', 1),
                      ('rx_crc_errors', 2),
                      ('rx_align_errors', 3),
                      ('tx_payload_bytes', 4),
                      ('rx_payload_bytes', 5),
                      ('tx_pause', 6),
                      ('rx_pause', 7),
                      ('rx_errors', 8),
                      ('tx_errors', 9),
                      ('rx_unicast', 10),
                      ('rx_multicast', 11),
                      ('rx_broadcast', 12),
                      ('tx_discards', 13),
                      ('tx_unicast', 14),
                      ('tx_multicast', 15),
                      ('tx_broadcast', 16),
                      ('ether_drops', 17),
                      ('rx_total_bytes', 18),
                      ('rx_total_packets', 19),
                      ('rx_undersize', 20),
                      ('rx_oversize', 21),
                      ('rx_64_bytes', 22),
                      ('rx_65_127_bytes', 23),
                      ('rx_128_255_bytes', 24),
                      ('rx_256_511_bytes', 25),
                      ('rx_512_1023_bytes', 26),
                      ('rx_1024_1518_bytes', 27),
                      ('rx_gte_1519_bytes', 28),
                      ('rx_jabbers', 29),
                      ('rx_runts', 30))

    def __init__(self, args):
        self._pcie_address = args.pcie_address
        self._hssi_grps = args.hssi_grps
        HSSICOMMON.__init__(self)

    def get_hssi_stats(self):
        """
        clear ctl address and ctl sts CSR
        write 0x3 value ctl address and address bit
        write read cmd 0x1 value ctl sts csr
        poll for status
        read LSB stats
        clear ctl address and ctl sts CSR
        write 0x3 value ctl address , address bit  and set 31 bit
        write read 0x1 value ctl sts csr
        poll for status
        read MSB stats
        print stats
        """
        self.open(self._hssi_grps[0][0])
        ctl_addr = hssi_ctl_addr(0)
        ctl_addr.sal_cmd = HSSI_SAL_CMD.READ_MAC_STATISTIC.value
        value = 0

        port_str = "{0: <32} |".format('HSSI Ports')
        hssi_feature_list = hssi_feature(self.read32(0, self.hssi_csr.HSSI_FEATURE_LIST))
        print("HSSI num ports:", hssi_feature_list.num_hssi_ports)
        for port in range(0, self.hssi_csr.HSSI_PORT_COUNT):
            # add active ports
            enable = self.register_field_get(hssi_feature_list.port_enable,
                                             port)
            if enable == 1:
                port_str += "port:{}|".format(port).rjust(20, ' ')

        print(port_str)

        stats_list = []
        for str, reg in self.hssi_eth_stats:
            stats_list.append("{0: <32} |".format(str))

        for port in range(0, self.hssi_csr.HSSI_PORT_COUNT):
            stat_index = 0
            # add active ports
            enable = self.register_field_get(hssi_feature_list.port_enable,
                                             port)
            if enable == 0:
                continue
            for str, reg in self.hssi_eth_stats:
                ctl_addr.value = 0
                ctl_addr.sal_cmd = HSSI_SAL_CMD.READ_MAC_STATISTIC.value

                # Read LSB value
                ctl_addr.addressbit = reg
                ctl_addr.port_address = port
                ctl_addr.value = self.register_field_set(ctl_addr.value,
                                                         31, 1, 1)
                res, value_lsb = self.read_reg(0, ctl_addr.value)
                if not res:
                    stats_list[stat_index] += "{}|".format("N/A").rjust(20, ' ')
                    stat_index = stat_index + 1
                    continue

                # Read MSB value
                ctl_addr.value = self.register_field_set(ctl_addr.value,
                                                         31, 1, 0)
                res, value_msb = self.read_reg(0, ctl_addr.value)
                if not res:
                    stats_list[stat_index] += "{}|".format("N/A").rjust(20, ' ')
                    stat_index = stat_index + 1
                    continue

                # 64 bit value
                value = (value_msb << 32) | (value_lsb)

                stats_list[stat_index] += "{}|".format(value).rjust(20, ' ')
                stat_index = stat_index + 1

        print("\n")
        for i in range(len(self.hssi_eth_stats)):
            print(stats_list[i])

        self.close()

        return 0

    def clear_hssi_stats(self):
        """
        clear ctl address and ctl sts CSR
        write 0x3 value ctl address and address bit
        write read cmd 0x1 value ctl sts csr
        poll for status
        read LSB stats
        clear ctl address and ctl sts CSR
        write 0x3 value ctl address , address bit  and set 31 bit
        write read 0x1 value ctl sts csr
        poll for status
        read MSB stats
        print stats
        """
        self.open(self._hssi_grps[0][0])
        ctl_addr = hssi_ctl_addr(0)
        ctl_addr.sal_cmd = HSSI_SAL_CMD.RESET_MAC_STATISTIC.value
        value = 0

        port_str = "{0: <32} |".format('HSSI Ports')
        hssi_feature_list = hssi_feature(self.read32(0, self.hssi_csr.HSSI_FEATURE_LIST))
        print("HSSI num ports:", hssi_feature_list.num_hssi_ports)
        print("HSSI stats clearing ...")
        for port in range(0, self.hssi_csr.HSSI_PORT_COUNT):
            # add active ports
            enable = self.register_field_get(hssi_feature_list.port_enable,
                                             port)
            if enable == 0:
                continue
            ctl_addr.value = 0
            ctl_addr.sal_cmd = HSSI_SAL_CMD.RESET_MAC_STATISTIC.value
            ctl_addr.port_address = port
            # set bit 16 and 17
            ctl_addr.value = self.register_field_set(ctl_addr.value,
                                                     16, 1, 1)
            ctl_addr.value = self.register_field_set(ctl_addr.value,
                                                     17, 1, 1)
            ret = self.clear_ctl_sts_reg(0)
            if not ret:
                print("Failed to clear HSSI CTL STS csr")
                return False
            self.write32(0, self.hssi_csr.HSSI_CTL_ADDRESS, ctl_addr.value)
            # write to ctl sts reg
            cmd_sts = hssi_cmd_sts(0x2)
            self.write32(0, self.hssi_csr.HSSI_CTL_STS, cmd_sts.value)
            time.sleep(HSSI_STATS_CLEAR_SLEEP_TIME)
            ret = self.clear_ctl_sts_reg(0)
            if not ret:
                print("Failed to clear HSSI CTL STS csr")
                return False
        print("HSSI stats cleared")

        self.close()
        return True

    def hssi_stats_start(self):
        """
        print hssi info
        get hssi stats
        """
        if not self.hssi_info(self._hssi_grps[0][0]):
            print("Failed to read hssi information")
            sys.exit(1)
        self.get_hssi_stats()

    def hssi_stats_clear(self):
        """
        print hssi info
        get hssi stats
        """
        if not self.hssi_info(self._hssi_grps[0][0]):
            print("Failed to read hssi information")
            sys.exit(1)

        if not self.clear_hssi_stats():
            print("hssi stats clearing failed")
            sys.exit(1)


def main():
    """
    parse input arguments pciaddress and mtu
    enum fpga pcie devices and find match
    read hssi statistics
    """
    parser = argparse.ArgumentParser()

    pcieaddress_help = 'sbdf of device to program \
                        (e.g. 0000:04:00.0).' \
                       ' Optional when one device in system.'
    parser.add_argument('--pcie-address', '-P',
                        default=None, help=pcieaddress_help)

    parser.add_argument('--clear', '-C', action='store_true',
                        help='clears hssi statistics')

    args = parser.parse_args()

    print(args)
    if args.pcie_address and not verify_pcie_address(args.pcie_address.lower()):
        sys.exit(1)

    args.hssi_grps = []
    f = FpgaFinder(args.pcie_address.lower() if args.pcie_address else None)
    devs = f.enum()
    if not devs:
        print('no FPGA found')
        sys.exit(1)

    for d in devs:
        print('sbdf: {segment:04x}:{bus:02x}:{dev:02x}.{func:x}'.format(**d))
        print('FPGA dev:', d)
        args.hssi_grps += f.find_hssi_group(d['pcie_address'])
    print("args.hssi_grps{}".format(args.hssi_grps))
    if len(args.hssi_grps) == 0:
        print("Failed to find HSSI feature")
        sys.exit(1)
    if len(args.hssi_grps) > 1:
        print('{} FPGAs are found: {}\nPlease choose one FPGA'
            .format(len(args.hssi_grps), [d[2] for d in args.hssi_grps]))
        sys.exit(1)

    print("fpga uio dev:{}".format(args.hssi_grps[0][0]))
    lp = FPGAHSSISTATS(args)
    if args.clear:
        lp.hssi_stats_clear()
    else:
        lp.hssi_stats_start()


if __name__ == "__main__":
    main()

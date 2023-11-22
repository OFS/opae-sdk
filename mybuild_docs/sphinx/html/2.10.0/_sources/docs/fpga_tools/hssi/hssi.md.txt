# hssi #

## SYNOPSIS ##

`hssi COMMON_OPTIONS MODE MODE_OPTIONS`

## DESCRIPTION ##

The ```hssi``` application provides a means of interacting with the 10G and
with the 100G HE-HSSI AFUs. In both 10G and 100G operating modes, the application
initializes the AFU and completes the desired transfer as described by the mode-
specific options.

COMMON_OPTIONS - application options common to both 10G and 100G modes.

`-h, --help`

    Display common command-line help and exit.

`-p, --pci-address ADDR`

    The PCIe address of the desired accelerator in ssss:bb:dd.f format.

`-s, --shared on|off`

    Whether to open the accelerator in shared mode. The default is off.

`-t, --timeout VALUE`

    The application timeout value in milliseconds. The default timeout is 60000 msec.

MODE - select AFU. Valid values are hssi_10g and hssi_100g.

MODE_OPTIONS [hssi_10g] - application options specific to the 10G AFU.

`-h, --help`

    Display 10G AFU specific command-line help and exit.

`--port PORT`

    Select the QSFP port in the range 0-7. The default is port 0.

`--eth-loopback on|off`

    Whether to enable loopback on the ethernet interface. Valid values are
    on and off. The default is on.

`--num-packets PACKETS`

    The number of packets to transfer. The default is 1 packet.

`--random-length fixed|random`

    Specify packet length randomization. Valid values are fixed and
    random. The default is fixed (no randomization).

`--random-payload incremental|random`

    Specify payload randomization. Valid values are incremental and
    random. The default is incremental.

`--packet-length LENGTH`

    Specify packet length. The default is 64 bytes.

`--src-addr ADDR`

    Specify the source MAC address. The default value is 11:22:33:44:55:66.

`--dest-addr ADDR`

    Specify the destination MAC address. The default value is 77:88:99:aa:bb:cc.

`--rnd-seed0 SEED0`

    Specify the prbs generator bits [31:0]. The default is 1592590336.

`--rnd-seed1 SEED1`

    Specify the prbs generator bits [47:32]. The default is 1592590337.

`--rnd-seed2 SEED2`

    Specify the prbs generator bits [91:64]. The default is 155373.

MODE_OPTIONS [hssi_100g] - application options specific to the 100G AFU.

`--port PORT`

    Select the QSFP port in the range 0-7. The default is port 0.

`--eth-loopback on|off`

    Whether to enable loopback on the ethernet interface. Valid values are
    on and off. The default is on.

`--num-packets PACKETS`

    The number of packets to transfer. The default is 1 packet.

`--gap random|none`

    Inter-packet gap. Valid values are random and none. The default is none.

`--pattern random|fixed|increment`

    Pattern mode. Valid values are random, fixed, or increment. The default
    is random.

`--src-addr ADDR`

    Specify the source MAC address. The default value is 11:22:33:44:55:66.

`--dest-addr ADDR`

    Specify the destination MAC address. The default value is 77:88:99:aa:bb:cc.

`--start-size SIZE`

    Specify the packet size in bytes, or the first packet size for --pattern increment.

`--end-size SIZE`

    Specify the end packet size in bytes.

`--end-select pkt_num|gen_idle`

    Specify packet generation end mode.

MODE_OPTIONS [pkt_filt_10g] - application options specific to the Packet Filter 10G AFU.

`--dfl-dev DFL_DEV`

    Packet Filter DFL device, eg --dfl-dev dfl_dev.0

MODE_OPTIONS [pkt_filt_100g] - application options specific to the Packet Filter 100G AFU.

`--dfl-dev DFL_DEV`

    Packet Filter DFL device, eg --dfl-dev dfl_dev.1

## EXAMPLES ##

`hssi -h`<br>
`hssi hssi_10g -h`<br>
`sudo hssi --pci-address=0000:3b:00.0 hssi_10g --eth-loopback=on --num-packets=500`<br>
`sudo hssi --pci-address=0000:3b:00.0 hssi_100g --pattern=increment`

## Revision History ##

Document Version | Intel Acceleration Stack Version | Changes
-----------------|----------------------------------|--------
2021.01.08 | IOFS EA | Initial release.

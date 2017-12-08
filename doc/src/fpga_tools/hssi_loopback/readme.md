# hssi_loopback #

## NAME ##
_hssi_loopback_ - Software utility to run HSSI loopback tests on FPGA


## SYNOPSIS ##
`hssi_loopback [[--bus|-b <bus number>] [--device | -d <device number>] [--function | -f <function number>]]|[--socket-id <socket-id>]
               [--mode|-m auto|e40|e10]
               [send [<source port> [<destination port>] [--packet-count|-c <count>] [--packet-delay|-d <delay>] [--packet-length|-l <length>]]
               |status [clear] | stop | readmacs`

## DESCRIPTION ##
hssi_loopback is a command line utility to interact with a packet generator GBS (green bitstream)
designed to test HSSI (high-speed serial interface) cards. The tests can be either external loopback or internal loopback.
External loopback is performed when two ports are specified. If only one port is specified, then internal loopback is performed.


_NOTE_: The following limitations apply to the current version of hssi_loopback

* When two ports are specified, they don't necassarily have to be different. For the e10 design, the ports should be the same
* The only GBS that hssi_loopback supports are the e40 and e10 E2E GBS (developed for testing HSSI with a retimer card)
* hssi_loopback is designed to work with CSRs defined in the AFU (Accelerator Functional Unit)
  interface of the GBS


## OPTIONS ##
`-S SOCKET_ID, --socket-id SOCKET_ID`
    Socket id of FPGA resource

`-B BUS, --bus BUS`
    Bus id of FPGA resource

`-D DEVICE, --device DEVICE`
    Device id of FPGA resource

`-F FUNCTION, --function FUNCTION`
    Function id of FPGA resource

`-G, --guid`
    specify what guid to use for the AFC enumeration

`-m, --mode`
    One of the following: [`auto`, `e40`, `e10]
    `auto` is the default and is used to indicate that the software try to run the mode based on the
    first AFC it enumerates.

`-t, --timeout`
    Timeout (in seconds) before the application terminates if run in continous mode. Continuous mode is selected
    when the number of packets is not specified.

`-y, --delay`
    Delay (in seconds) between printing out a simple status line. Default is 0.100 seconds (100 milliseconds).

`-c, --packet-count`
    The number of packets to send.

`-d, --packet-delay`
    The delay in between packets. This delay is the number of cyles of a 100MHz clock Roughly about 10 nanoseconds.

`-s, --packet-size`
    The packet size to send where the minimum is 46 bytes and the maximum is 1500 bytes. Default is 46 bytes.

## COMMANDS ##
`send <source port> [<destination port>] [--packet-count|-c <count>] [--packet-delay|-d <delay>] [--packet-length|-l <length>]`
    Send packets from one port to the other. If the destination port is omitted then an internal loopback is perfomed.
    Otherwise, an external loopback is performed from source port to destination port.

`status [clear]`
    Read and interpret the status registers to print to the screen.
    `clear` is used to indicate that the status registers should be cleared.

`stop`
    Issue a stop command to all Ethernet controllers in the GBS.

`readmacs`
    Read and display the MAC addresses of the ports. This is stored in the EEPROM.

## EXIT CODES ##
    0     Success - Number of packets received are equal to the number of packets sent and no errors
          are reported.

    -1    Loopback failure - Either number of packets don't match or errors were detected

    -2    Errors parsing arguments

## EXAMPLES ##
Read the mac addresses of the GBS loaded on bus 0x5e:

```sh
>sudo hssi_loopback readmacs -B 0x5e
```

Run an external loopback, sending 100 packets from port 0 to port 1. The GBS has been loaded on bus 0x5e

```sh
>sudo hssi_loopback -B 0x5e send 0 1 -c 100
```

Run an internal loopback until a timeout of 5 seconds has been reached. The GBS has been loaded on bus 0x5e

```sh
>sudo hssi_loopback -B 0x5e send 0 -t 5

```

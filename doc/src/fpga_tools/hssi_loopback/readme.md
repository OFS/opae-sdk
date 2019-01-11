# hssi_loopback #

## NAME ##
_hssi_loopback_ - Software utility to run HSSI loopback tests on FPGA


## SYNOPSIS ##
`hssi_loopback [[--bus|-b <bus number>] [--device | -d <device number>] [--function | -f <function number>]]|[--socket-id <socket-id>]
      [--mode|-m auto|e40|e10] 
      [send [<source port> [<destination port>] [--packet-count|-c <count>] [--packet-delay|-d <delay>] [--packet-length|-l <length>]] |status [clear] | stop | readmacs`

## DESCRIPTION ##

The ```hssi_loopback``` utility works in conjunction with a packet generator accelerator function unit (AFU)
to test high-speed serial interface (HSSI) cards. The ```hssi_loopback``` utility tests both external and internal loopbacks.
```hssi_loopback``` runs an external loopback test when the command line arguments include both source and destination ports.
```hssi_loopback``` runs an internal loopback test when command line arguments include a single port. ```hssi_loopback```
only runs on the Intel Xeon with Arria 10 FPGA. You cannot run it on the Intel PAC (programmable accelerator card).


_NOTE_: The following limitations apply to the current version of hssi_loopback:

* For the external loopback the two port arguments can be the same. For the e10 design, the ports should be the same.
* The ```hssi_loopback``` test supports only the e40 and e10 E2E AFUs.  The e10 E2E AFU tests HSSI with a retimer card.
* The ```hssi_loopback``` test uses the control and status registers (CSRs) defined in the AFU.

## OPTIONS ##
`-S SOCKET_ID, --socket-id SOCKET_ID`
 
 Socket ID FPGA resource.

`-B BUS, --bus BUS`

Bus ID of FPGA resource.

`-D DEVICE, --device DEVICE`

Device ID of FPGA resource.

`-F FUNCTION, --function FUNCTION`

Function ID of FPGA resource.

`-G, --guid`

Specifies guid for the AFC enumeration.

`-m, --mode`
    
One of the following: [`auto`, `e40`, `e10`]
`auto` is the default and indicates that the software runs the mode based on the first accelerator functional
context (AFC) it enumerates.

`-t, --timeout`

Timeout (in seconds) before the application terminates in continuous mode. Continuous mode is the default
when you do not specify the number of packets.

`-y, --delay`

Delay (in seconds) between printing out a simple status line. Default is 0.100 seconds (100 milliseconds).

`-c, --packet-count`

The number of packets to send.

`-d, --packet-delay`

The delay in between packets. This delay is the number of 100 MHz clock cycles, roughly 10 nanoseconds.

`-s, --packet-size`

The packet size to send. The minimum is 46 bytes and the maximum is 1500 bytes. The default is 46 bytes.

## COMMANDS ##
`send <source port> [<destination port>] [--packet-count|-c <count>] [--packet-delay|-d <delay>] [--packet-length|-l <length>]`

Send packets from one port to the other. If the command line does not specify a destination port, the test runs an internal 
loopback. Otherwise, the test runs an external loopback from the source port to the destination port.

`status [clear]`

Read and interpret the status registers and print to the screen. `clear` clears the status registers.

`stop`

Issue a stop command to all Ethernet controllers in the AFU.

`readmacs`

Read and display the port MAC addresses. An EEPROM stores the MAC addresses.

## EXIT CODES ##

0    Success - Number of packets received are equal to the number of packets sent and no errors
          are reported.

-1    Loopback failure - Either number of packets does not match or the test detected errors.

-2    Errors parsing arguments.

## EXAMPLES ##

Read the MAC addresses of the AFU loaded on bus 0x5e:

```sh
>sudo hssi_loopback readmacs -B 0x5e
```

Run an external loopback, sending 100 packets from port 0 to port 1. The AFU is on bus 0x5e:

```sh
>sudo hssi_loopback -B 0x5e send 0 1 -c 100
```

Run an internal loopback until a timeout of 5 seconds is reached. The AFU is on bus 0x5e:

```sh
>sudo hssi_loopback -B 0x5e send 0 -t 5

```
## Revision History ##

 | Document Version |  Intel Acceleration Stack Version  | Changes  |
 | ---------------- |------------------------------------|----------|
 | 2018.05.21 | 1.1 Beta. <br>(Supported with Intel Quartus Prime Pro Edition 17.1.) | Corrected typos.  | 

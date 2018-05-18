# mmlink #

## Synopsis  ##

`mmlink [-B <bus>] [-D <device>] [-F <function>] [-S <socket>] [-P <TCP port>] [-I <IP Address>]`


## Description ##
The Remote Signal Tap logic analyzer provides real-time hardware debugging for the Accelerator Function Unit (AFU). 
It provides a signal trace capability that the Quartus Prime software adds to the AFU. The Remote Signal Tap logic
analyzer provides access to the Remote Signal Tap part of the Port MMIO space and then runs the remote protocol.

## Examples  ##

`./mmlink  -B 0x5e -P 3333`

  MMLink app starts and listens for connection.

## Options ##

`-B,--bus` 

FPGA Bus number.

`-D,--device` 

FPGA Device number.

`-F,--function` 

FPGA function number.

`-S,--socket` 

FPGA socket number.

`-P,--port` 

TCP port number.

`-I,--ip ` 

IP address of FPGA system. 


## Notes ##

Driver privilege:

Change AFU driver privilege to user:

```
$ chmod 777 /dev/intel-fpga-port.0
```


Change locked memory size:

edit the file /etc/security/limits.conf

```
$ sudo vi /etc/security/limits.conf

user    hard   memlock           10000

user    soft   memlock           10000
```

Exit terminal and log into a new terminal.

Verify that the locked memory is now set: 
```
$ ulimit -l 10000

## Revision History ##

 | Document Version |  Intel Acceleration Stack Version  | Changes  |
 | ---------------- |------------------------------------|----------|
 | 2018.05.21 | 1.1 Beta. <br>(Supported with Intel Quartus Prime Pro Edition 17.1.) | No changes from previous release.  | 


# mmlink #

## SYNOPSIS  ##

`mmlink [-B <bus>] [-D <device>] [-F <function>] [-S <socket>] [-P <TCP port>] [-I <IP Address>]`


## DESCRIPTION ##

Remote signaltap is software  tool used for debugging the Accelerator Function Unit (AFU), effectively a signal trace capability that Quartus places into a green bitstream.
Remote Signal Tap provides  access the RST part of the Port MMIO space, and then runs the remote protocol on top.

## EXAMPLES  ##

`./mmlink  -B 0x5e -P 3333`

  MMLink app starts and listens for connection.

## OPTIONS ##

`-B,--bus` FPGA Bus number.

`-D,--device` FPGA Device number.

`-F,--functio` FPGA function number.

`-S,--socket` FPGA socket number.

`-P,--port` TCP port number.

`-I,--ip ` IP address of FPGA system. 


## NOTES ##

Driver privilege:

Change AFU driver privilege to user.

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

exit terminal and login new terminal.

verify that the locked memory is now set: 
```
$ ulimit -l 10000



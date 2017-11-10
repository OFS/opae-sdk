# fpga_errors.py #

## SYNOPSIS ##
```console
fpga_errors.py [-h] [-s SOCKET_ID] [-b BUS] [-d DEVICE] [-f FUNCTION]
               [-c]
               <resource>
```

## DESCRIPTION ##
fpga_errors.py is used to query and print out any errors in the FPGA resources.
It reads the error values from sysfs and decodes them per the SAS to print out
human readable output. Additionally, it can be used to clear the errors.

.. note::

```
  The script must be run as root to clear the errors.
```

## RESOURCES ##
Valid resource specification are as follows:

    fme
    port
    first_error
    pcie0
    pcie1
    bbs
    gbs
    all - print/clear errors on all resources listed above

## OPTIONS ##
    -s SOCKET_ID, --socket-id SOCKET_ID
       socket id of FPGA resource

    -b BUS, --bus BUS
       bus id of FPGA resource

    -d DEVICE, --device DEVICE
       device id of FPGA resource


    -f FUNCTION, --function FUNCTION
       function id of FPGA resource

    -c, --clear
       specify whether or not to clear error registers


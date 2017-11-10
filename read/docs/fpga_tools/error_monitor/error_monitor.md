# error_monitor #

This guide contains information about the following:

  fpga_errors.py Utility 


## Usage ##

fpga_errors.py is used to query and print out any errors in the FPGA resources. It reads the error values from sysfs and decodes them per the SAS to print out human readable output. Additionally, it can be used to clear the errors.

.. note::

```
  The script must be run as root to clear the errors.
```

```console
usage: fpga_errors.py [-h] [-s SOCKET_ID] [-b BUS] [-d DEVICE] [-f FUNCTION]
                      [-c]
                      {fme,port,first_error,pcie0,pcie1,bbs,gbs,all}
```

Positional Arguments:

<table>
    <tr>
    <td>{fme,port,first_error,pcie0,pcie1,bbs,gbs,all}</td>
    <td>specify what kind of errors to operate on</td>
    </tr>
</table>

Optional Arguments:

<table>
    <tr>
    <td>-h, --help</td>
    <td>show this help message and exit</td>
    </tr>

    <tr>
    <td>-s SOCKET_ID, --socket-id SOCKET_ID</td>
    <td>socket id of FPGA resource</td>
    </tr>

    <tr>
    <td>-b BUS, --bus BUS</td>
    <td>bus id of FPGA resource</td>
    </tr>

    <tr>
    <td>-d DEVICE, --device DEVICE</td>
    <td>device id of FPGA resource</td>
    </tr>


    <tr>
    <td>-f FUNCTION, --function FUNCTION</td>
    <td>function id of FPGA resource</td>
    </tr>

    <tr>
    <td>-c, --clear</td>
    <td>specify whether or not to clear error registers</td>
    </tr>
</table>


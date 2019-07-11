# fpgad #

## SYNOPSIS ##
`fpgad --daemon [--logfile=<file>] [--pidfile=<file>] [--socket=<sock>] [--null-bitstream=<file>]`
`fpgad [--socket=<sock>] [--null-bitstream=<file>]`

## DESCRIPTION ##
```fpgad``` periodically monitors and reports the error status reflected in the device driver's error status sysfs files.
```fpgad``` establishes the channel to communicate events to the Open Programmable Accelerator Engine (OPAE) application. 
```fpgad``` programs a NULL bitstream in response to an AP6 (power) event.

If your system does not support interrupts, you must run ```fpgad``` before the API calls `fpgaRegisterEvent` and
`fpgaUnregisterEvent` can succeed.

Use SIGINT to stop ```fpgad```.

`-d, --daemon`

    When specified, fpgad executes as a system daemon process.

`-l, --logfile <file>`

    Send output to log file. If omitted, fpgad uses /var/lib/opae/fpgad.log.

`-p, --pidfile <file>`

    When starting up, fpgad writes its process id to a file.
    If omitted, fpgad uses /var/lib/opae/fpgad.pid.

`-s, --socket <sock>`

    Listen for event API registration requests on the UNIX domain socket on the specified path. 
    The default=/tmp/fpga_event_socket. 

`-n, --null-bitstream <file>`

    Specify the NULL bitstream to program when an AP6 event occurs. This option may be specified multiple
    times. The AFU, if any, that matches the FPGA's PR interface ID is programmed when an AP6
    event occurs. This option is not applicable for the N3000 PAC.

`-c, --config <file>`

    Specify fpgad's configuration file. By default, fpgad uses /etc/opae/fpgad.cfg.

## TROUBLESHOOTING ##

If you encounter any issues, you can get debug information by examining the log file.

## EXAMPLES ##

`sudo systemctl start fpgad`

## Notes for the N3000 PCIe&reg; Accelerator Card (PAC) ##

```fpgad``` periodically monitors each of the Board Management Controller's on-board sensors.
If a sensor supports a high-warn or low-warn threshold and that threshold is met, ```fpgad```
responds by disabling AER for the PAC. AER is disabled in order to avoid a system reset in
the case that a sensor values reaches the high-fatal or low-fatal threshold. When high-fatal
or low-fatal is reached, the Board Management Controller removes power from the PAC in
order to avoid damage. When power is removed from the PAC, the kernel experiences a surprise
device removal and responds by resetting the system. Disabling AER prior to the Board
Management Controller's powering down the PAC avoids the surprise device removal and subsequent
system reset.

fpgad's configuration file provides a mechanism for the user to specify additional sensor
monitoring in the case that the Board Management Controller does not provide high-warn or
low-warn thresholds for the sensor. To enable this feature, set the configuration file's
"config-sensors-enabled" key to true, and specify the desired sensor and thresholds.

For example:

  "config-sensors-enabled": true,
  "sensors": [
    {
      "id": 25,
      "low-warn": 11.40,
      "low-fatal": 10.56
    }
  ]

 ## Revision History ##
    
 | Document Version |  Intel Acceleration Stack Version  | Changes  |
 | ---------------- |------------------------------------|----------|
 |2018.05.21 | 1.1 Beta. <br>(Supported with Intel Quartus Prime Pro Edition 17.1.1.) | No changes from previous release. |
 |2019.07.10 | N3000 1.1 Alpha2. | Update fpgad options. Describe N3000 sensor monitoring. |

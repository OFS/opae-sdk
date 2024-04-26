# qsfpinfo #

## SYNOPSIS  ##

`qsfpinfo [-h] [-p <port>] [-S <segment>] [-B <bus>] [-D <device>] [-F <function>] [PCI_ADDR]`


## DESCRIPTION ##

qsfpinfo prints QSFP EEPROM information for QSFP modules present in cards with qsfp feature (id 0x13) e.g. N6010, N6011, using the qsfp-mem driver.

qsfpinfo only prints QSFP EEPROM information if OPAE-SDK is compiled with  "-DOPAE_WITH_QSFPINFO_QSFPPRINT=ON".
## EXAMPLES  ##

```sh
>sudo qsfpinfo -p 0 0000:98:00.0`
PCIe s:b:d.f                                      : 0000:98:00.0
QSFP0                                             : Connected
	Identifier                                : 0x11 (QSFP28)
	Extended identifier                       : 0xcc
	Extended identifier description           : 3.5W max. Power consumption
	Extended identifier description           : CDR present in TX, CDR present in RX
	Extended identifier description           : High Power Class (> 3.5 W) not enabled
	Power set                                 : Off
	Power override                            : Off
	Connector                                 : 0x0c (MPO Parallel Optic)
	Transceiver codes                         : 0x80 0x00 0x00 0x00 0x00 0x00 0x00 0x00
	Transceiver type                          : 100G Ethernet: 100G Base-SR4 or 25GBase-SR
	Encoding                                  : 0x07 ((256B/257B (transcoded FEC-enabled data))
	BR, Nominal                               : 25500Mbps
	Rate identifier                           : 0x00
	Length (SMF,km)                           : 0km
	Length (OM3 50um)                         : 70m
	Length (OM2 50um)                         : 0m
	Length (OM1 62.5um)                       : 0m
	Length (Copper or Active cable)           : 50m
	Transmitter technology                    : 0x00 (850 nm VCSEL)
	Laser wavelength                          : 850.000nm
	Laser wavelength tolerance                : 10.000nm
	Vendor name                               : FINISAR CORP
	Vendor OUI                                : 00:90:65
	Vendor PN                                 : FTLC9551REPM
	Vendor rev                                : A0
	Vendor SN                                 : XYH0RPZ
	Date code                                 : 180512
	Revision Compliance                       : SFF-8636 Rev 2.5/2.6/2.7
	Rx loss of signal                         : None
	Tx loss of signal                         : None
	Rx loss of lock                           : [ Yes, Yes, Yes, Yes ]
	Tx loss of lock                           : [ Yes, Yes, Yes, Yes ]
	Tx fault                                  : None
	Module temperature                        : 45.22 degrees C / 113.39 degrees F
	Module voltage                            : 3.3359 V
	Alarm/warning flags implemented           : Yes
	Laser tx bias current (Channel 1)         : 8.162 mA
	Laser tx bias current (Channel 2)         : 7.560 mA
	Laser tx bias current (Channel 3)         : 7.796 mA
	Laser tx bias current (Channel 4)         : 7.708 mA
	Transmit avg optical power (Channel 1)    : 1.0391 mW / 0.17 dBm
	Transmit avg optical power (Channel 2)    : 1.0704 mW / 0.30 dBm
	Transmit avg optical power (Channel 3)    : 1.0704 mW / 0.30 dBm
	Transmit avg optical power (Channel 4)    : 0.9449 mW / -0.25 dBm
	Rcvr signal avg optical power(Channel 1)  : 0.9603 mW / -0.18 dBm
	Rcvr signal avg optical power(Channel 2)  : 0.9210 mW / -0.36 dBm
	Rcvr signal avg optical power(Channel 3)  : 1.0281 mW / 0.12 dBm
	Rcvr signal avg optical power(Channel 4)  : 1.0220 mW / 0.09 dBm
...
```

## OPTIONS ##

`-p,--port`

Print only information for this QSFP [0..1]

`-S,--segment`

FPGA segment number.

`-B,--bus`

FPGA Bus number.

`-D,--device`

FPGA Device number.

`-F,--function`

FPGA function number.

## Revision History ##

| Date | Intel Acceleration Stack Version | Changes Made |
|:------|----------------------------|:--------------|
|2024.04.26| IOFS EA | Initial release. |


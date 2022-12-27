# ofs.uio #

## SYNOPSIS ##
`ofs.uio [-h] [--pcie-address PCIE_ADDRESS] [--uio uiox] [--feature-id FEATURE_ID] [--region-index REGION_INDEX]
                  [--mailbox-cmdcsr offset] [--bit-size {8,16,32,64}] [--peek offset] [--poke offset value]
                  [--mailbox-read offset] [--mailbox-dump address size] [--mailbox-write address value]`<br>

`ofs.uio [--uio uiox] [--peek offset]`<br>
`ofs.uio [--uio uiox] [--poke offset value] `<br>
`ofs.uio [--uio uiox] [--mailbox-read address] `<br>
`ofs.uio [--uio uiox] [--mailbox-write address value] `<br>
`ofs.uio [--uio uiox] [--mailbox-dump address size] `<br>
`ofs.uio [--pcie-address PCIE_ADDRESS] [--feature-id FEATURE_ID] [--peek offset]`<br>
`ofs.uio [--pcie-address PCIE_ADDRESS] [--feature-id FEATURE_ID] [--poke offset value] `<br>
`ofs.uio [--pcie-address PCIE_ADDRESS] [--feature-id FEATURE_ID] [--mailbox-read address] `<br>
`ofs.uio [--pcie-address PCIE_ADDRESS] [--feature-id FEATURE_ID] [--mailbox-write address value] `<br>
`ofs.uio [--pcie-address PCIE_ADDRESS] [--feature-id FEATURE_ID] [--mailbox-dump address size] `<br>


## DESCRIPTION ##

```ofs.uio``` is a tool that provides user space access to DFL UIO devices,
command line options like peek, poke, mailbox-read, mailbox-write, mailbox-dump to 
access Configuration and Status Registers (CSRs).


##  OPTIONS ##

#### Peek ####
Peek/Read UIO CSR offset<br>
`ofs.uio [--uio uio] [--peek offset]`<br>
`ofs.uio [--pcie-address PCIE_ADDRESS] [--feature-id FEATURE_ID] [--peek offset]`<br>

#### Poke ####
Poke/Write value to UIO CSR offset<br>
`ofs.uio [--uio uio] [--poke offset value] `<br>
`ofs.uio [--pcie-address PCIE_ADDRESS] [--feature-id FEATURE_ID] [--poke offset value] `<br>

#### Mailbox Read ####
Read CSR address using mailbox<br>
`ofs.uio [--uio uio] [--mailbox-read address] `<br>
`ofs.uio [--pcie-address PCIE_ADDRESS] [--feature-id FEATURE_ID] [--mailbox-read address] `<br>

#### Mailbox Write ####
Write value to CSR address using mailbox <br>
`ofs.uio [--uio uio] [--mailbox-write address value] `<br>
`ofs.uio [--pcie-address PCIE_ADDRESS] [--feature-id FEATURE_ID] [--mailbox-write address value] `<br>

#### Mailbox Dump ####
Reads/Dumps block size of CSR address using mailbox<br>
`ofs.uio [--uio uio] [--mailbox-dump address size] `<br>
`ofs.uio [--pcie-address PCIE_ADDRESS] [--feature-id FEATURE_ID] [--mailbox-dump address size] `<br>

#### Bit size ####
Read/Write bit-field 8,16,32,64 sizes<br>
`ofs.uio [--uio uio] --bit-size 8 [--peek offset]`<br>
`ofs.uio [--uio uio] --bit-size 32 [--peek offset]`<br>

#### PCIe Address ####
PCIE_ADDR PCIe address of FPGA device<br>
`ofs.uio [--pcie-address PCIE_ADDRESS] [--feature-id FEATURE_ID] [--peek offset]`<br>

#### UIO region index ####
UIO region index, default region index is 0 <br> 
`ofs.uio [--uio uio] --region-index 0 [--peek offset]`<br>
`ofs.uio [--uio uio] --region-index 1 [--peek offset]`<br>

#### Mailbox command status csr offset ####
Mailbox command status csr offset, 
default value set to dfl pcie subsystem system feature mailbox command status register offset 0x28 <br> 
`ofs.uio [--uio uio] --mailbox-cmdcsr 0xa8 [--mailbox-read address] `<br>
`ofs.uio [--pcie-address PCIE_ADDRESS] [--feature-id FEATURE_ID] --mailbox-cmdcsr 0xa8  [--mailbox-read address] `<br>

## EXAMPLES ##
Peek/Read
```
ofs.uio --uio uio0 --peek 0x0
peek(0x0): 0x3000000010002015

ofs.uio --uio uio6 --peek 0x0
peek(0x0): 0x3000000100000020

ofs.uio --pcie-address 0000:b1:00.0 --feature-id 0x15 --peek 0x0
peek(0x0): 0x3000000010002015

ofs.uio --uio uio0 --peek 0x0 --bit-size 32
peek(0x0): 0x10002015
```

Poke/Write
```
ofs.uio --uio uio6 --peek 0x8
peek(0x8): 0x0
ofs.uio --uio uio6 --poke  0x8 0xabcdd12345
poke(0x8):0xabcdd12345

ofs.uio --pcie-address 0000:b1:00.0 --feature-id 0x15 --peek 0x0
peek(0x8): 0x0
ofs.uio --pcie-address 0000:b1:00.0 --feature-id 0x15 --poke  0x8 0x1234
poke(0x8):0x1234
```

Mailbox Read
```
ofs.uio --uio uio6 --mailbox-read 0x0
MailboxRead(0x0): 0x1000000
ofs.uio --uio uio6 --mailbox-read 0x8
MailboxRead(0x8): 0x110c000

ofs.uio --pcie-address 0000:b1:00.0 --feature-id 0x20 --mailbox-read 0x0
MailboxRead(0x0): 0x1000000
ofs.uio --pcie-address 0000:b1:00.0 --feature-id 0x20 --mailbox-read 0x8 
MailboxRead(0x8): 0x110c000
```

Mailbox Write
```
ofs.uio --uio uio6 --mailbox-write 0x0 0x1234
MailboxWrite(0x0):0x1234
ofs.uio --uio uio6 --mailbox-read 0x0
MailboxRead(0x0):0x1234

ofs.uio --pcie-address 0000:b1:00.0 --feature-id 0x20 --mailbox-write 0x0 0x1234
MailboxWrite(0x0):0x1234
ofs.uio --pcie-address 0000:b1:00.0 --feature-id 0x20 --mailbox-read 0x0 
MailboxRead(0x0):0x1234
```

Mailbox Dump
```
ofs.uio --uio uio6 --mailbox-dump 0x0 0x10
MailboxDump(0x0): 0x1000000
MailboxDump(0x4): 0x1000000
MailboxDump(0x8): 0x110c000
MailboxDump(0xc): 0x110c000
MailboxDump(0x10): 0x0
MailboxDump(0x14): 0x0
MailboxDump(0x18): 0x0
MailboxDump(0x1c): 0x0
MailboxDump(0x20): 0x0
MailboxDump(0x24): 0x0
MailboxDump(0x28): 0x0
MailboxDump(0x2c): 0x0
MailboxDump(0x30): 0x0
MailboxDump(0x34): 0x0
MailboxDump(0x38): 0x0
MailboxDump(0x3c): 0x0



ofs.uio --pcie-address 0000:b1:00.0 --feature-id 0x20 --mailbox-dump 0x0 0x10
MailboxDump(0x0): 0x1000000
MailboxDump(0x4): 0x1000000
MailboxDump(0x8): 0x110c000
MailboxDump(0xc): 0x110c000
MailboxDump(0x10): 0x0
MailboxDump(0x14): 0x0
MailboxDump(0x18): 0x0
MailboxDump(0x1c): 0x0
MailboxDump(0x20): 0x0
MailboxDump(0x24): 0x0
MailboxDump(0x28): 0x0
MailboxDump(0x2c): 0x0
MailboxDump(0x30): 0x0
MailboxDump(0x34): 0x0
MailboxDump(0x38): 0x0
MailboxDump(0x3c): 0x0


```


## Revision History ##

Document Version | Intel Acceleration Stack Version | Changes
-----------------|----------------------------------|--------
2021.01.25 | IOFS EA | Initial release.

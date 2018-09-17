### README

### Streaming DMA AFU
The streaming DMA AFU implements read-master and
write-master DMA channels.
The read-master DMA issues reads on the Avalon-MM port and
writes on the Avalon-ST port. The write-master DMA issues
reads on the Avalon-MM port and writes on the Avalon-ST port.
In the reference configuration provided, Avalon-ST port of the read
master DMA drives a pattern checker and Avalon-ST port of the 
write master DMA is driven from a pattern generator. Refer to the
Streaming DMA user guide for a detailed description of the
hardware architecture.

### Software Driver Model
Each master appears to the software driver as a DMA channel.
The channel must be opened using fpgaDmaOpen before use.
Both read and write channels can be operated independently.
The driver processes transfers on a channel in the order
they were issued. The driver does not offer
any ordering guarantee on transfers issued across channels.

Each DMA transfer operation is represented by a transfer object
of type fpga_dma_transfer_t. User applications must properly initialize
and configure this object before the transfer is issued. 
The driver blocks user applications
from manipulating attributes of the transfer object after
the transfer is initiated.

The driver supports deterministic and non-determinstic
length transfers. Use deterministic-length transfers when
the total size of the transfer can be determined before the 
transfer is initiated. Non-deterministic length 
transfers must signal end of the stream using TX/RX
control attributes in the transfer object.

The driver supports synchronous (blocking) and asynchronous
(non-blocking) transfers. Asynchronous transfers
return immediately to the caller. The caller is notified using
registered callback after the transfer is complete. 
On the other hand, synchronous 
transfers return to the caller only after the operation 
is complete. Note that if no callback is specified, the
transfer is considered synchronous.

The software driver supports several features:

* Discovery of available channels by scanning device header
list (fpgaCountDMAChannels)
* Open/close a DMA channel by index (fpgaDmaOpen/Close)
* Query channel properties - TX or RX. By convention, the TX
channel transfers data from host memory to streaming FPGA port
and the RX channel transfers data from streaming FPGA port to
host memory.
* Initialize and destroy a transfer object (fpgaDMATransferInit/
Destroy).
* Set transfer attributes such as source address, destination
address, length, transmit and receive control (fpgaDMATransferSet\*).
* Query transfer attributes such as the total number of bytes
transferred.
* Initiate transfer and register callback for completion
notification (fpgaDMATransfer)

### Compiling the driver
```
$ make prefix=<path to OPAE install>
```

### ASE Simulation
ASE simulation is supported

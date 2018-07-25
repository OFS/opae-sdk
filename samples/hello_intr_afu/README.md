# Simple Interrupt AFU example

hello_intr_afu triggers an interrupt in hw/rtl/afu.sv that is handled by the 
software. 
 
 
Also of note: the top-level interface includes the legacy DDR4 wires that were 
the default interface in earlier releases.  While AFU developers are encouraged 
to switch to the new interface, the wired version remains available. 
hello_intr_afu configures the top-level by setting the afu-top-interface class 
to ccip_std_afu_avalon_mm_legacy_wires in hw/rtl/hello_intr_afu.json. 


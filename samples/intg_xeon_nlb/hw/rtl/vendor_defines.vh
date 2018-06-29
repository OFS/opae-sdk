// ***************************************************************************
// Copyright (c) 2013-2016, Intel Corporation
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
// * Neither the name of Intel Corporation nor the names of its contributors
// may be used to endorse or promote products derived from this software
// without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
// ***************************************************************************
// ------------------------------------------------------------------------
//  TOOL and VENDOR Specific configurations
// ------------------------------------------------------------------------
// The TOOL and VENDOR definition necessary to correctly configure PAR project
// package currently supports
// Vendors : Xilinx & Altera
// Tools   : Synplify & Quartus II & Vivado
`include "sys_cfg_pkg.svh"
`ifndef VENDOR_DEFINES_VH
`define VENDOR_DEFINES_VH

    // Generate error if Vendor not defined
    `ifdef VENDOR_XILINX
            `ifdef VENDOR_ALTERA
                    ***Select only one VENDOR option***
            `endif
    `else
            `ifndef VENDOR_ALTERA
                    ***Select atleast one VENDOR option***
            `endif        
    `endif
    
    `ifdef VENDOR_ALTERA
        `define GRAM_AUTO "no_rw_check"                         // defaults to auto
        `define GRAM_BLCK "no_rw_check, M20K"
        `define GRAM_DIST "no_rw_check, MLAB"
    `endif
    
    //-------------------------------------------
    // Generate error if TOOL not defined
    `ifdef TOOL_QUARTUS
            `ifdef TOOL_SYNPLIFY
                    ***Select only one TOOL option***
            `endif
            `ifdef TOOL_VIVADO
                    ***Select only one TOOL option***
            `endif
    
    `elsif TOOL_SYNPLIFY
            `ifdef TOOL_QUARTUS
                    ***Select atleast one TOOL option***
            `endif        
            `ifdef TOOL_VIVADO
                    ***Select atleast one TOOL option***
            `endif        
    `else
            `ifndef TOOL_VIVADO
                    ***Select atleast one TOOL option***
            `endif                
    `endif
    
    `ifdef TOOL_QUARTUS
        `define GRAM_STYLE ramstyle
        `define NO_RETIMING  dont_retime
        `define NO_MERGE dont_merge
        `define KEEP_WIRE syn_keep = 1
    `endif
    
    `ifdef TOOL_SYNPLIFY
        `define GRAM_STYLE syn_ramstyle
        `define NO_RETIMING syn_allow_retiming=0
        `define NO_MERGE syn_preserve=1
        `define KEEP_WIRE syn_keep=1
    
        `ifdef VENDOR_XILINX
            `define GRAM_AUTO "no_rw_check"
            `define GRAM_BLCK "block_ram"
            `define GRAM_DIST "select_ram"
        `endif
    
    `endif 
    
    `ifdef TOOL_VIVADO  
        `define GRAM_STYLE ram_style
        `define NO_RETIMING dont_touch="true"
        `define NO_MERGE dont_touch="true"
        `define KEEP_WIRE keep="true"
    
        `ifdef VENDOR_XILINX
            `define GRAM_AUTO "auto_gram"
            `define GRAM_BLCK "block"
            `define GRAM_DIST "distributed"
        `endif
    `endif 


`endif

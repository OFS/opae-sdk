//
// Copyright (c) 2017, Intel Corporation
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
//
// Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// Neither the name of the Intel Corporation nor the names of its contributors
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

//
// Wire together two avalon_mem_if instances.
//
module avalon_mem_if_connect
   (
    avalon_mem_if.to_fiu mem_fiu,
    avalon_mem_if.to_afu mem_afu
    );

    always_comb
    begin
        mem_afu.waitrequest = mem_fiu.waitrequest;
        mem_afu.readdata = mem_fiu.readdata;
        mem_afu.readdatavalid = mem_fiu.readdatavalid;

        mem_fiu.burstcount = mem_afu.burstcount;
        mem_fiu.writedata = mem_afu.writedata;
        mem_fiu.address = mem_afu.address;
        mem_fiu.write = mem_afu.write;
        mem_fiu.read = mem_afu.read;
        mem_fiu.byteenable = mem_afu.byteenable;

        // Debugging signal
        mem_afu.bank_number = mem_fiu.bank_number;
    end

endmodule // avalon_mem_if_connect

// Copyright(c) 2017, Intel Corporation
//
// Redistribution  and  use  in source  and  binary  forms,  with  or  without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of  source code  must retain the  above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// * Neither the name  of Intel Corporation  nor the names of its contributors
//   may be used to  endorse or promote  products derived  from this  software
//   without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO,  THE
// IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT  SHALL THE COPYRIGHT OWNER  OR CONTRIBUTORS BE
// LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
// CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT LIMITED  TO,  PROCUREMENT  OF
// SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  DATA, OR PROFITS;  OR BUSINESS
// INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY  OF LIABILITY,  WHETHER IN
// CONTRACT,  STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE  OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.



// $File: //acds/rel/17.0/ip/sopc/components/verification/altera_avalon_clock_source/altera_avalon_clock_source.sv $
// $Revision: #1 $
// $Date: 2017/02/12 $
// $Author: swbranch $
//------------------------------------------------------------------------------
// Clock generator

`timescale 1ps / 1ps

module altera_avalon_clock_source (clk);
   output clk;

   parameter int unsigned CLOCK_RATE = 10;           // clock rate in MHz / kHz / Hz depends on the clock unit
   parameter              CLOCK_UNIT = 1000000;      // clock unit MHz / kHz / Hz

// synthesis translate_off
   import verbosity_pkg::*;

   localparam time HALF_CLOCK_PERIOD = 1000000000000.000000/(CLOCK_RATE*CLOCK_UNIT*2); // half clock period in ps

   logic clk = 1'b0;

   string message   = "*uninitialized*";
   string freq_unit = (CLOCK_UNIT == 1)? "Hz" :
                      (CLOCK_UNIT == 1000)? "kHz" : "MHz";
   bit    run_state = 1'b1;

   function automatic void __hello();
      $sformat(message, "%m: - Hello from altera_clock_source.");
      print(VERBOSITY_INFO, message);
      $sformat(message, "%m: -   $Revision: #1 $");
      print(VERBOSITY_INFO, message);
      $sformat(message, "%m: -   $Date: 2017/02/12 $");
      print(VERBOSITY_INFO, message);
      $sformat(message, "%m: -   CLOCK_RATE = %0d %s", CLOCK_RATE, freq_unit);
      print(VERBOSITY_INFO, message);
      print_divider(VERBOSITY_INFO);
   endfunction

   function automatic string get_version();  // public
      // Return BFM version as a string of three integers separated by periods.
      // For example, version 9.1 sp1 is encoded as "9.1.1".
      string ret_version = "17.0";
      return ret_version;
   endfunction

   task automatic clock_start();  // public
      // Turn the clock on. By default the clock is initially turned on.
      $sformat(message, "%m: Clock started");
      print(VERBOSITY_INFO, message);
      run_state = 1;
   endtask

   task automatic clock_stop();  // public
      // Turn the clock off.
      $sformat(message, "%m: Clock stopped");
      print(VERBOSITY_INFO, message);
      run_state = 0;
   endtask

   function automatic get_run_state();  // public
      // Return the state of the clock source: running=1, stopped=0
      return run_state;
   endfunction

   initial begin
      __hello();
   end

   always begin
      #HALF_CLOCK_PERIOD;
      clk = run_state;

      #HALF_CLOCK_PERIOD;
      clk = 1'b0;
   end
// synthesis translate_on

endmodule


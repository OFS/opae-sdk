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



// $Id: //acds/rel/17.0/ip/sopc/components/verification/lib/verbosity_pkg.sv#1 $
// $Revision: #1 $
// $Date: 2017/02/12 $
//-----------------------------------------------------------------------------
// =head1 NAME
// verbosity_pkg
// =head1 SYNOPSIS
// Package for controlling verbosity of messages sent to the console
//-----------------------------------------------------------------------------
// =head1 DESCRIPTION
// This module will dump diagnostic messages to the console during
// simulation. The level of verbosity can be controlled in the test
// bench by using the *set_verbosity* method in the imported package
// verbosity_pkg. For a given setting, message at that level and all
// lower levels are dumped. For example, setting VERBOSITY_DEBUG level
// causes all messages to be dumped, while VERBOSITY_FAILURE restricts
// only failure messages and those tagged as VERBOSITY_NONE to be
// dumped.
// The different levels are:
// =over 4
// =item 1 VERBOSITY_NONE
// Messages tagged with this level are always dumped to the console.
// =item 2 VERBOSITY_FAILURE
// A fatal simulation error has occurred and the simulator will exit.
// =item 3 VERBOSITY_ERROR
// A non-fatal error has occured. An example is a data comparison mismatch.
// =item 4 VERBOSITY_WARNING
// Warn the user that a potential error has occurred.
// =item 5 VERBOSITY_INFO
// Informational message.
// =item 6 VERBOSITY_DEBUG
// Dump enough state to diagnose problem scenarios.
// =back


`ifndef _AVALON_VERBOSITY_PKG_
`define _AVALON_VERBOSITY_PKG_

package verbosity_pkg;

	timeunit 1ps;
	timeprecision 1ps;

   typedef enum int {VERBOSITY_NONE,
		     VERBOSITY_FAILURE,
		     VERBOSITY_ERROR,
		     VERBOSITY_WARNING,
		     VERBOSITY_INFO,
		     VERBOSITY_DEBUG} Verbosity_t;

   Verbosity_t  verbosity = VERBOSITY_INFO;
   string 	message = "";
   int 		dump_file;
   int 		dump = 0;

   //--------------------------------------------------------------------------
   // =head1 Public Methods API
   // =pod
   // This section describes the public methods in the application programming
   // interface (API). In this case the application program is the test bench
   // or component which imports this package.
   // =cut
   //--------------------------------------------------------------------------

   function automatic Verbosity_t get_verbosity(); // public
      // Returns the global verbosity setting.
      return verbosity;
   endfunction

   function automatic void set_verbosity ( // public
      Verbosity_t v
   );
      // Sets the global verbosity setting.

      string       verbosity_str;
      verbosity = v;

      case(verbosity)
	VERBOSITY_NONE: verbosity_str = "VERBOSITY_";
	VERBOSITY_FAILURE: verbosity_str = "VERBOSITY_FAILURE";
	VERBOSITY_ERROR: verbosity_str = "VERBOSITY_ERROR";
        VERBOSITY_WARNING: verbosity_str = "VERBOSITY_WARNING";
    	VERBOSITY_INFO: verbosity_str = "VERBOSITY_INFO";
    	VERBOSITY_DEBUG: verbosity_str = "VERBOSITY_DEBUG";
	default: verbosity_str = "UNKNOWN";
      endcase
      $sformat(message, "%m: Setting Verbosity level=%0d (%s)",
               verbosity, verbosity_str);
      print(VERBOSITY_NONE, message);
   endfunction

   function automatic void print( // public
      Verbosity_t level,
      string message
   );
      // Print a message to the console if the verbosity argument
      // is less than or equal to the global verbosity setting.
      string level_str;

      if (level <= verbosity) begin
	 case(level)
	   VERBOSITY_NONE: level_str = "";
	   VERBOSITY_FAILURE: level_str = "FAILURE:";
	   VERBOSITY_ERROR: level_str = "ERROR:";
           VERBOSITY_WARNING: level_str = "WARNING:";
    	   VERBOSITY_INFO: level_str = "INFO:";
    	   VERBOSITY_DEBUG: level_str = "DEBUG:";
	   default: level_str = "UNKNOWN:";
	 endcase

	 $display("%t: %s %s",$time, level_str, message);
	 if (dump) begin
	    $fdisplay(dump_file, "%t: %s %s",$time, level_str, message);
	 end
      end
   endfunction

   function automatic void print_divider( // public
      Verbosity_t level
   );
      // Prints a divider line to the console to make a block of related text
      // easier to identify and read.
      string message;
      $sformat(message,
	       "------------------------------------------------------------");
      print(level, message);
   endfunction

   function automatic void open_dump_file ( // public
      string dump_file_name = "avalon_bfm_sim.log"
   );
      // Opens a dump file which collects console messages.

      if (dump) begin
	 $sformat(message, "%m: Dump file already open - ignoring open.");
	 print(VERBOSITY_ERROR, message);
      end else begin
	 dump_file = $fopen(dump_file_name, "w");
	 $fdisplay(dump_file, "testing dump file");
	 $sformat(message, "%m: Opening dump file: %s", dump_file_name);
	 print(VERBOSITY_INFO, message);
	 dump = 1;
      end
   endfunction

   function automatic void close_dump_file();  // public
      // Close the console message dump file.
      if (!dump) begin
	 $sformat(message, "%m: No open dump file - ignoring close.");
	 print(VERBOSITY_ERROR, message);
      end else begin
	 dump = 0;
	 $fclose(dump_file);
	 $sformat(message, "%m: Closing dump file");
	 print(VERBOSITY_INFO, message);
      end
   endfunction

   function automatic void abort_simulation();
      string message;
      $sformat(message, "%m: Abort the simulation due to fatal error incident.");
      print(VERBOSITY_FAILURE, message);
      $stop;
   endfunction

endpackage

// =cut

`endif


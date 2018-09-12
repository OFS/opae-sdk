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

#include <unistd.h>
#include <iostream>
#include <getopt.h>
#include <json-c/json.h>
//#include <types_int.h>
#include "gtest/gtest.h"

using ::testing::TestCase;
using ::testing::TestInfo;
using ::testing::TestPartResult;
using ::testing::UnitTest;

using namespace std;


/**
* @brief      Class for global options.
*/
class GlobalOptions {
public:
	static GlobalOptions& Instance();

	void NumSockets(unsigned s) { m_NumSockets = s; }
	unsigned NumSockets() const { return m_NumSockets; }

	void Bus(signed b) { m_Bus = b; }
	signed Bus() const { return m_Bus; }

	void VM(bool bvm) { m_VM = bvm; }
	bool VM() { return m_VM; }

	void Platform(unsigned p) { m_Platform = p; }
	unsigned Platform() const { return m_Platform; }

protected:
	static GlobalOptions sm_Instance;

	unsigned m_NumSockets;
	signed m_Bus;
	unsigned m_Platform;
	bool m_VM;

	GlobalOptions() : m_NumSockets(1), m_Bus(0), m_Platform(0), m_VM(false) {}

};

struct option longopts[] = {{"help", no_argument, NULL, 'h'},
                            {"bus", required_argument, NULL, 'b'},
                            {"virt", no_argument, NULL, 'v'},                            
                            {0, 0, 0, 0}};


void show_help() {
	cout << "Usage: gtapi" << endl
	     << endl
	     << "\t-b,--bus <bus> number." << endl
	     << "\t-v,--virt <VM> disable auto-reconfiguration of NLB0." << endl
	     << "\t-j,--json <path> provide path to bitstream json file." << endl
	     << "\t-p,--platform <ase/fpga> select which platform to run test."
	     << endl
	     << endl;
}

int main(int argc, char** argv) {
	int getopt_ret;
	int option_index;

	while ((getopt_ret = getopt_long(argc, argv, ":hb:vj:p:", longopts,
					 &option_index)) != -1) {
		const char* tmp_optarg = optarg;

		switch (getopt_ret) {
		case 'h':
			show_help();
			::testing::InitGoogleTest(&argc, argv);
			return 0;
			break;

		case 'b':
			if (strcmp(tmp_optarg, "all") == 0) {
				//GlobalOptions::Instance().Bus(-1);
				break;
			} else {
				//GlobalOptions::Instance().Bus((signed)strtol(tmp_optarg, NULL, 16));
			}
			break;

		case 'v':
			//GlobalOptions::Instance().VM(true);
			cout << "Disabling auto reconfiguration." << endl;
			break;

		case ':':
			cout << "Missing option argument.";
			return 1;
			break;

		case '?':
		default:;  // Let the option pass through to Google Test.
		}
	}

	::testing::InitGoogleTest(&argc, argv);
	//fetchConfiguration(path);

	signed retval = RUN_ALL_TESTS();
	
	exit(retval);
	return 0; // unreachable code
}

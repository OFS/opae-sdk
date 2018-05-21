// Copyright(c) 2017-2018, Intel Corporation
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
#include <common_utils.h>
#include "gtest/gtest.h"

using ::testing::TestCase;
using ::testing::TestInfo;
using ::testing::TestPartResult;
using ::testing::UnitTest;

using namespace std;
using namespace common_utils;

void clean() {
	for (auto& item : config_map) {
		free(item.second);
	}
}

struct option longopts[] = {{"help", no_argument, NULL, 'h'},
                            {"bus", required_argument, NULL, 'b'},
                            {"virt", no_argument, NULL, 'v'},
                            {"json", required_argument, NULL, 'j'},
                            {"plaform", required_argument, NULL, 'p'},
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

	const char* path;
	path = "../tests/configuration.json";

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
				GlobalOptions::Instance().Bus(-1);
				break;
			} else {
				GlobalOptions::Instance().Bus((signed)strtol(tmp_optarg, NULL, 16));
			}
			break;

		case 'v':
			GlobalOptions::Instance().VM(true);
			cout << "Disabling auto reconfiguration." << endl;
			break;

		case 'j':
			cout << "Path to configuration json file " << tmp_optarg << endl;
			path = tmp_optarg;
			break;

		case 'p':
			if (strcmp(tmp_optarg, "ase") == 0) {
				GlobalOptions::Instance().Platform(1);
			} else {
				GlobalOptions::Instance().Platform(0);
			}
			cout << "Select Platform: " << GlobalOptions::Instance().Platform()
			     << endl;
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
	fetchConfiguration(path);

	signed retval = RUN_ALL_TESTS();
	signed i = atexit(clean);
	if (i) exit(EXIT_FAILURE);

	exit(retval);
	return 0; // unreachable code
}

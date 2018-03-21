/*++

  INTEL CONFIDENTIAL
  Copyright 2016 - 2017 Intel Corporation

  The source code contained or described  herein and all documents related to
  the  source  code  ("Material")  are  owned  by  Intel  Corporation  or its
  suppliers  or  licensors.  Title   to  the  Material   remains  with  Intel
  Corporation or  its suppliers  and licensors.  The Material  contains trade
  secrets  and  proprietary  and  confidential  information  of Intel  or its
  suppliers and licensors.  The Material is protected  by worldwide copyright
  and trade secret laws and treaty provisions. No part of the Material may be
  used,   copied,   reproduced,   modified,   published,   uploaded,  posted,
  transmitted,  distributed, or  disclosed in  any way  without Intel's prior
  express written permission.

  No license under any patent, copyright,  trade secret or other intellectual
  property  right  is  granted to  or conferred  upon  you by  disclosure  or
  delivery of the  Materials, either  expressly, by  implication, inducement,
  estoppel or otherwise. Any license  under such intellectual property rights
  must be express and approved by Intel in writing.

  --*/

#include <unistd.h>
#include <iostream>
#include <getopt.h>
#include <json-c/json.h>
#include <common_test.h>
#include <types_int.h>
#include "gtest/gtest.h"

using ::testing::TestCase;
using ::testing::TestInfo;
using ::testing::TestPartResult;
using ::testing::UnitTest;

using namespace std;
using namespace common_test;

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

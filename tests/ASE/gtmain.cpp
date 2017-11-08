#include <unistd.h>
#include "gtest/gtest.h"
#include <iostream>
#include "globals.h"

#include <getopt.h>

using ::testing::TestCase;
using ::testing::TestInfo;
using ::testing::TestPartResult;
using ::testing::UnitTest;
using namespace std;

struct option longopts[] = {
   {"help",    no_argument,       NULL, 'h'},
   {"sockets", required_argument, NULL, 's'},
   {0, 0, 0, 0}
};

void show_help()
{
   cout << "Usage: gtapi" << endl
	<< "\t-s,--sockets <sockets> number of sockets on the test system." << endl
	<< endl;

}

int main( int argc, char** argv )
{
   int getopt_ret;
   int option_index;
   while ( ( getopt_ret = getopt_long(argc, argv, ":hs:", longopts, &option_index) ) != -1 ) {
      const char *tmp_optarg = optarg;

      if((optarg) &&
	 ('=' == *tmp_optarg)) {
         ++tmp_optarg;
      }

      if((!optarg) &&
	 (NULL != argv[optind]) &&
         ('-' != argv[optind][0]) ) {
         tmp_optarg = argv[optind++];
      }

      switch ( getopt_ret ) {
         case 'h':
	    show_help();
	    ::testing::InitGoogleTest(&argc, argv);
	    return 0;
         break;

	 case 's':
            GlobalOptions::Instance().NumSockets((unsigned)strtoul(tmp_optarg, NULL, 0));
            cout << "Setting num sockets to " << GlobalOptions::Instance().NumSockets() << endl;
         break;

         case ':' :
            cout << "Missing option argument.";
            return 1;
         break;

         case '?' :
         default  :
            ; // Let the option pass through to Google Test.
      }

   }

   ::testing::InitGoogleTest(&argc, argv);
   return RUN_ALL_TESTS();
}

GlobalOptions GlobalOptions::sm_Instance;
GlobalOptions & GlobalOptions::Instance() { return GlobalOptions::sm_Instance; }


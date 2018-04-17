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


int main( int argc, char** argv )
{
   ::testing::InitGoogleTest(&argc, argv);
   return RUN_ALL_TESTS();
}

GlobalOptions GlobalOptions::sm_Instance;
GlobalOptions & GlobalOptions::Instance() { return GlobalOptions::sm_Instance; }


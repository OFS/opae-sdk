/*
 * gtsysfs.cpp
 * Copyright (C) 2018 rrojo <rrojo@ub-rojo-vm-04.amr.corp.intel.com>
 *
 * Distributed under terms of the MIT license.
 */
#include "gtest/gtest.h"

using ::testing::TestCase;
using ::testing::TestInfo;
using ::testing::TestPartResult;
using ::testing::UnitTest;

int main(int argc, char* argv[])
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

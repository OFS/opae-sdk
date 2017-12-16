#include "gtest/gtest.h"

#include "opaec++/log.h"

using namespace opae::fpga::internal;


/**
 * @test log_error
 * Given a logger object with a given name
 * When I log an error message
 * Then a formatted log messages is printed to stderr
 */
TEST(CxxLog, log_error) {
  logger log("the_name");
  testing::internal::CaptureStderr();
  log.error() << "ThIs IS aN erroR StrIng";
  std::string msg = testing::internal::GetCapturedStderr();
  EXPECT_NE(msg.find("[ERROR][the_name] ThIs IS aN erroR StrIng"), std::string::npos);
}


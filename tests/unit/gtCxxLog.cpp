#include "gtest/gtest.h"

#include <opae/cxx/core/log.h>

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

/**
 * @test error_if_true
 * Given a logger object with a given name
 * When I call error_if with a true condition
 * Then a formatted log messages is printed to stderr
 */
TEST(CxxLog, error_if_true) {
  logger log("the_name");
  testing::internal::CaptureStderr();
  log.error_if(true) << "ThIs IS aN erroR StrIng";
  std::string msg = testing::internal::GetCapturedStderr();
  EXPECT_NE(msg.find("[ERROR][the_name] ThIs IS aN erroR StrIng"), std::string::npos);
}

/**
 * @test error_if_false
 * Given a logger object with a given name
 * When I call error_if with a false condition
 * And I read from stderr
 * Then stderr is empty
 */
TEST(CxxLog, warn_if_false) {
  logger log("the_name");
  testing::internal::CaptureStderr();
  log.error_if(false) << "ThIs IS aN erroR StrIng";
  std::string msg = testing::internal::GetCapturedStderr();
  EXPECT_TRUE(msg.empty());
}

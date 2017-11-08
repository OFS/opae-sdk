#include "gtest/gtest.h"
#include "globals.h"
#include "option_map.h"
#include "option_parser.h"

extern int optind;

using namespace intel::utils;

/**
 * @test default_option_01
 * Given an options map with one option (with no default value)
 * When I try to get the value
 * The option value is the default value for that type
 */
TEST(Options, default_option_01)
{
    option_map opts;
    opts.add_option<uint32_t>("answer", option::with_argument, "test option help");
    EXPECT_NE(42, opts["answer"]->value<uint32_t>());
    EXPECT_EQ(0, opts["answer"]->value<uint32_t>());
}

/**
 * @test parse_json_02
 * Given an options map with one option (with a default value)
 * And a JSON string with the value in it
 * When I parse the string
 * The option value is updated to the value in the JSON string
 */
TEST(Options, parse_json_02)
{
    option_map opts;
    opts.add_option<uint32_t>("answer", option::with_argument, "test option help", 42);
    EXPECT_EQ(42, opts["answer"]->value<uint32_t>());
    option_parser parser;
    ASSERT_TRUE(parser.parse_json("{\"answer\": 43}", opts));

    EXPECT_NE(42, opts["answer"]->value<uint32_t>());
    EXPECT_EQ(43, opts["answer"]->value<uint32_t>());
}


/**
 * @test parse_json_03
 * Given an options map with one option (with a default value)
 * And a JSON string with the value not in it
 * When I parse the string
 * The option value is not updated to the value in the JSON string
 * And the is_set function returns false
 */
TEST(Options, parse_json_03)
{
    option_map opts;
    opts.add_option<uint32_t>("answer", option::with_argument, "test option help", 42);
    EXPECT_EQ(42, opts["answer"]->value<uint32_t>());
    option_parser parser;
    ASSERT_TRUE(parser.parse_json("{\"not_answer\": 43}", opts));

    EXPECT_NE(43, opts["answer"]->value<uint32_t>());
    EXPECT_EQ(42, opts["answer"]->value<uint32_t>());
    EXPECT_FALSE(opts["answer"]->is_set());
}

/**
 * @test parse_arg_04
 * Given an options map with one option (with a default value)
 * And the option has no short option
 * And a command line argument vector with the long option
 * When I parse the arguments vector
 * The option value is updated to the value in the arguments vector
 */
TEST(Options, parse_arg_04)
{
    optind = 1;
    option_map opts;
    opts.add_option<uint32_t>("answer", option::with_argument, "test option help", 42);
    EXPECT_EQ(42, opts["answer"]->value<uint32_t>());
    option_parser parser;
    char* args[] ={"program", "--answer=43", nullptr};

    ASSERT_TRUE(parser.parse_args(2, args, opts));

    EXPECT_NE(42, opts["answer"]->value<uint32_t>());
    EXPECT_EQ(43, opts["answer"]->value<uint32_t>());
}

/**
 * @test parse_arg_05
 * Given an options map with one option (with a default value)
 * And the option has a short option
 * And a command line argument vector with the short option
 * When I parse the arguments vector
 * The option value is updated to the value in the arguments vector
 */
TEST(Options, parse_arg_05)
{
    optind = 1;
    option_map opts;
    opts.add_option<uint32_t>("answer", 'a', option::with_argument, "test option help", 42);
    EXPECT_EQ(42, opts["answer"]->value<uint32_t>());
    option_parser parser;
    char* args[] ={"program", "-a" ,"43", nullptr};

    ASSERT_TRUE(parser.parse_args(3, args, opts));

    EXPECT_NE(42, opts["answer"]->value<uint32_t>());
    EXPECT_EQ(43, opts["answer"]->value<uint32_t>());
}

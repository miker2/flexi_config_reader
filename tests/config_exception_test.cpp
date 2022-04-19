#include <gtest/gtest.h>

#include <filesystem>
#include <string>
#include <string_view>
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>

#include "config_actions.h"
#include "config_exceptions.h"
#include "config_grammar.h"
#include "config_reader.h"
#include "logger.h"

namespace {

template <typename INPUT>
auto parse(INPUT& input) {
  logger::setLevel(logger::Severity::WARN);
  config::ActionData out;
  return peg::parse<peg::must<config::grammar>, config::action>(input, out);
}

template <>
auto parse(std::filesystem::path& in_file) {
  peg::file_input cfg_file(in_file);
  return parse(cfg_file);
}

}  // namespace

TEST(config_exception_test, parse_error) {
  {
    const std::vector parse_error = {
        "struct test1 {                                                      \n\
           key =    # Missing value produces a parse_error                   \n\
         }",
        "foo.bar = 1   # Can't mix flat keys with struct in same file.       \n\
         struct test1 {                                                      \n\
           bar = 0                                                           \n\
         }",
        "struct test1 {                                                      \n\
           bar = 0                                                           \n\
         }                                                                   \n\
         foo.bar = 1   # Can't mix flat keys with struct in same file."};
    for (const auto& input : parse_error) {
      peg::memory_input in_cfg(input, "From content");
      EXPECT_THROW(parse(in_cfg), peg::parse_error) << "Input file: " << in_cfg.source();
    }
  }
}

TEST(config_exception_test, mismatched_key) {
  {
    const std::string_view mismatched_key =
        "\n\
        struct test1 {    \n\
          key = \"value\" \n\
        }  # This end key should match the 'struct' key, and will produce an exception.";
    peg::memory_input in_cfg(mismatched_key, "No trailing newline");
    EXPECT_THROW(parse(in_cfg), peg::parse_error) << "Input file: " << in_cfg.source();
  }
}

TEST(config_exception_test, duplicate_key) {
  {
    const std::vector in_str = {
        "struct test1 {      \n\
           key1 = \"value\"  \n\
           key2 = 0x10       \n\
           key1 = -4   #  Duplicate key. This should produce an exception. \n\
         }\n",
        "struct test2 { \n\
           struct foo { \n\
             bar = 0    \n\
           }            \n\
                        \n\
           struct foo { # This key is duplicated & leads to an exception \n\
             baz = 1    \n\
           }            \n\
         }\n"};
    size_t cnt{1};
    for (const auto& input : in_str) {
      peg::memory_input in_cfg(input, "From content: " + std::to_string(cnt++));
      EXPECT_THROW(parse(in_cfg), config::DuplicateKeyException)
          << "Input file: " << in_cfg.source();
    }
    {
      const std::string_view ref_proto_failure =
          "proto proto_foo {              \n\
             bar = 0                      \n\
             baz = $BAZ                   \n\
           }                              \n\
                                          \n\
           reference proto_foo as test2 { \n\
             $BAZ = \"baz\"               \n\
             +bar = 0                     \n\
                                          \n\
           }\n";
      ConfigReader cfg;
      EXPECT_THROW(cfg.parse(ref_proto_failure, "ref_proto_failure"),
                   config::DuplicateKeyException);
    }
  }
}

class CyclicReference : public testing::TestWithParam<std::string> {};

TEST_P(CyclicReference, Exception) {
  ConfigReader cfg;
  const auto in_file = std::filesystem::path(EXAMPLE_DIR) / "invalid" / GetParam();
  EXPECT_THROW(cfg.parse(in_file), config::CyclicReferenceException) << "Input file: " << in_file;
}

INSTANTIATE_TEST_SUITE_P(CyclicFiles, CyclicReference,
                         testing::Values("config_cyclic1.cfg", "config_cyclic2.cfg"));

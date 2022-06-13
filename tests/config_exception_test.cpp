#include <fmt/format.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <string>
#include <string_view>
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>

#include "common/logger.h"
#include "config/config_actions.h"
#include "config/config_exceptions.h"
#include "config/config_grammar.h"
#include "config/config_reader.h"

namespace {

template <typename INPUT>
auto parse(INPUT& input) {
  logger::setLevel(logger::Severity::WARN);
  config::ActionData out;
  return peg::parse<peg::must<config::grammar>, config::action>(input, out);
}

template <>
auto parse(std::filesystem::path& input) {
  peg::file_input cfg_file(input);
  return parse(cfg_file);
}

}  // namespace

// NOLINTNEXTLINE
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
      // NOLINTNEXTLINE(cppcoreguidelines-avoid-goto)
      EXPECT_THROW(parse(in_cfg), peg::parse_error) << "Input file: " << in_cfg.source();
    }
  }
}

// NOLINTNEXTLINE
TEST(config_exception_test, mismatched_key) {
  {
    const std::string_view mismatched_key =
        "\n\
        struct test1 {    \n\
          key = \"value\" \n\
        }  # This end key should match the 'struct' key, and will produce an exception.";
    peg::memory_input in_cfg(mismatched_key, "No trailing newline");
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-goto)
    EXPECT_THROW(parse(in_cfg), peg::parse_error) << "Input file: " << in_cfg.source();
  }
}

// NOLINTNEXTLINE
TEST(config_exception_test, DuplicateKeyException) {
  {
    const std::string_view duplicate_in_struct =
        "struct test1 {      \n\
           key1 = \"value\"  \n\
           key2 = 0x10       \n\
           key1 = -4   #  Duplicate key. This should produce an exception. \n\
         }\n";
    peg::memory_input in_cfg(duplicate_in_struct, "key1 defined twice");
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-goto)
    EXPECT_THROW(parse(in_cfg), config::DuplicateKeyException) << "Input file: " << in_cfg.source();
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
         }\n";
    ConfigReader cfg;
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-goto)
    EXPECT_THROW(cfg.parse(ref_proto_failure, "ref_proto_failure"), config::DuplicateKeyException);
  }
#if 0  // See FULLPAIR action
  {
    const std::string_view duplicate_full_pair =
        "this.is.a.key = 10         \n\
         another.key = -1.2         \n\
         this.is.a.key = \"again\"  \n";
    peg::memory_input in_cfg(duplicate_full_pair, "this.is.a.key defined twice");
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-goto)
    EXPECT_THROW(parse(in_cfg), config::DuplicateKeyException);
  }
#endif
  {
    const std::string_view var_add_duplicate =
        "proto proto_foo {              \n\
           bar = 0                      \n\
           baz = $BAZ                   \n\
         }                              \n\
                                        \n\
         reference proto_foo as test2 { \n\
           +bar = -1.2                  \n\
           $BAZ = \"baz\"               \n\
           +bar = 0                     \n\
         }\n";
    peg::memory_input in_cfg(var_add_duplicate, "test2.bar defined twice");
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-goto)
    EXPECT_THROW(parse(in_cfg), config::DuplicateKeyException);
  }
  {
    const std::string_view proto_pair_duplicate =
        "proto proto_foo {              \n\
           baz = $BAZ                   \n\
           bar = 0                      \n\
           baz = $(duplicate.key)       \n\
         }\n";
    peg::memory_input in_cfg(proto_pair_duplicate, "proto_foo.baz defined twice");
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-goto)
    EXPECT_THROW(parse(in_cfg), config::DuplicateKeyException);
  }
}

namespace {
auto structLikes() -> const std::vector<std::string_view>& {
  static const std::vector<std::string_view> struct_likes = {
      "struct fizz {                \n\
         buzz = \"buzz\"            \n\
       }",
      "reference bar as fizz {      \n\
         $BUZZ = \"buzz\"           \n\
       }",
      "proto fizz {                 \n\
         buzz = $BUZZ               \n\
       }"};
  return struct_likes;
}
}  // namespace

class DuplicateKeys
    : public testing::TestWithParam<std::tuple<std::string_view, std::string_view>> {};

// NOLINTNEXTLINE
TEST_P(DuplicateKeys, Exception) {
  const std::string_view cfg_base =
      "struct foo {{                    \n\
           bar = 0                        \n\
           baz = -1.2                     \n\
                                          \n\
           {}                             \n\
                                          \n\
           {}                             \n\
         }}\n";

  const std::string cfg = fmt::vformat(
      cfg_base, fmt::make_format_args(std::get<0>(GetParam()), std::get<1>(GetParam())));
  // std::cout << "Test config: \n" << cfg << std::endl;
  peg::memory_input in_cfg(cfg, "duplicate 'fizz'");
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-goto)
  EXPECT_THROW(parse(in_cfg), config::DuplicateKeyException) << "Input config:\n" << cfg;
}

// NOLINTNEXTLINE
INSTANTIATE_TEST_SUITE_P(config_exception_test, DuplicateKeys,
                         testing::Combine(testing::ValuesIn(structLikes()),
                                          testing::ValuesIn(structLikes())));

// NOLINTNEXTLINE
TEST(config_exception_test, InvalidKeyException) {
  {
    const std::string_view invalid_key_reference =
        // Here we have 'key' referencing a key 'test1.key2' that doesn't exist.
        "struct test1 {         \n\
           key = $(test1.key2)  \n\
         }\n";
    ConfigReader cfg;
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-goto)
    EXPECT_THROW(cfg.parse(invalid_key_reference, "invalid key reference"),
                 config::InvalidKeyException);
  }
  {
    // This can also occur if the key exists but is of the wrong type
    const std::string_view not_a_struct =
        // Here we have 'key' referencing a key 'test1.key3.bar' where 'test.key3' doesn't exist.
        "struct test1 {             \n\
           key = $(test1.key3.bar)  \n\
         }\n";
    ConfigReader cfg;
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-goto)
    EXPECT_THROW(cfg.parse(not_a_struct, "not a struct"), config::InvalidKeyException);
  }
}

// NOLINTNEXTLINE
TEST(config_exception_test, InvalidTypeException) {
  {
    // This can also occur if the key exists but is of the wrong type
    const std::string_view key_wrong_type =
        // Here we have 'key' referencing a key 'test1.key3.bar' where 'test.key3' doesn't exist.
        "struct test1 {             \n\
           key = $(test1.key3.bar)  \n\
           key3 = 0                 \n\
         }\n";
    ConfigReader cfg;
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-goto)
    EXPECT_THROW(cfg.parse(key_wrong_type, "key wrong type"), config::InvalidTypeException);
  }
  {
    const std::string_view non_numeric_in_expression =
        "struct foo {                      \n\
           key1 = \"not a number\"         \n\
           key2 = {{ 0.5 * $(foo.key1) }}  \n\
         }\n";
    ConfigReader cfg;
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-goto)
    EXPECT_THROW(cfg.parse(non_numeric_in_expression, "non numeric in expression"),
                 config::InvalidTypeException);
  }
}

// NOLINTNEXTLINE
TEST(config_exception_test, UndefinedReferenceVarException) {
  {
    const std::string_view proto_var_doesnt_exist =
        "proto foo_proto {             \n\
           key1 = $KEY1                \n\
           key2 = $KEY2                \n\
         }                             \n\
                                       \n\
         reference foo_proto as foo {  \n\
           $KEY1 = 0                   \n\
           #$KEY2 undefined            \n\
         }\n";
    ConfigReader cfg;
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-goto)
    EXPECT_THROW(cfg.parse(proto_var_doesnt_exist, "$KEY2 is undefined"),
                 config::UndefinedReferenceVarException);
  }
  {
    const std::string_view extra_proto_var =
        "proto foo_proto {                                 \n\
           key1 = $KEY1                                    \n\
           key2 = $KEY2                                    \n\
         }                                                 \n\
                                                           \n\
         reference foo_proto as foo {                      \n\
           $KEY1 = 0                                       \n\
           $KEY2 = \"defined\"                             \n\
           $EXTRA_KEY = 0  # not useful, but not an error  \n\
         }\n";
    ConfigReader cfg;
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-goto)
    EXPECT_NO_THROW(cfg.parse(extra_proto_var, "$EXTRA_KEY is unused"));
  }
}

// NOLINTNEXTLINE
TEST(config_exception_test, UndefinedProtoException) {
  {
    const std::string_view undefined_proto =
        "proto foo_proto {                                 \n\
           key1 = $KEY1                                    \n\
           key2 = $KEY2                                    \n\
         }                                                 \n\
                                                           \n\
         reference bar_proto as bar {                      \n\
           $KEY1 = 0                                       \n\
           $KEY2 = \"defined\"                             \n\
         }\n";
    ConfigReader cfg;
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-goto)
    EXPECT_THROW(cfg.parse(undefined_proto, "bar_proto not defined"),
                 config::UndefinedProtoException);
  }
}

class CyclicReference : public testing::TestWithParam<std::string> {};

// NOLINTNEXTLINE
TEST_P(CyclicReference, Exception) {
  ConfigReader cfg;
  const auto in_file = std::filesystem::path(EXAMPLE_DIR) / "invalid" / GetParam();
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-goto)
  EXPECT_THROW(cfg.parse(in_file), config::CyclicReferenceException) << "Input file: " << in_file;
}

// NOLINTNEXTLINE
INSTANTIATE_TEST_SUITE_P(config_exception_test, CyclicReference,
                         testing::Values("config_cyclic1.cfg", "config_cyclic2.cfg"));

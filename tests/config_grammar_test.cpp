#include <fmt/format.h>
#include <gtest/gtest.h>
#include <unistd.h>

#include <algorithm>
#include <iosfwd>
#include <optional>
#include <tao/pegtl/contrib/analyze.hpp>

#include "flexi_cfg/config/actions.h"
#include "flexi_cfg/config/classes.h"
#include "flexi_cfg/config/parser-internal.h"

// NOLINTBEGIN(google-readability-avoid-underscore-in-googletest-name)

namespace peg = TAO_PEGTL_NAMESPACE;

namespace {
using RetType = std::pair<bool, flexi_cfg::config::ActionData>;

template <typename GTYPE, template <typename...> class Control = peg::normal>
auto runTest(const std::string& test_str) -> RetType {
  peg::memory_input in(test_str, "from_content");
  flexi_cfg::config::ActionData out(std::filesystem::path(EXAMPLE_DIR));
  const auto ret =
      flexi_cfg::config::internal::parseCore<GTYPE, flexi_cfg::config::action, Control>(in, out);
  return {ret, out};
}

template <typename GRAMMAR, typename T>
void checkResult(const std::string& input, flexi_cfg::config::types::Type expected_type,
                 std::optional<flexi_cfg::config::ActionData>& out) {
  std::optional<RetType> ret;
  ASSERT_NO_THROW(ret.emplace(runTest<GRAMMAR>(input))) << "INPUT: '" << input << "'";
  ASSERT_TRUE(ret.has_value());
  if (ret.has_value()) {
    ASSERT_TRUE(ret.value().first);
    EXPECT_EQ(ret.value().second.obj_res->type, expected_type);
    const auto value = dynamic_pointer_cast<T>(ret.value().second.obj_res);
    ASSERT_NE(value, nullptr);
  }

  if (ret.has_value()) {
    out = ret.value().second;
  }
}
}  // namespace

TEST(ConfigGrammar, analyze) { ASSERT_EQ(peg::analyze<flexi_cfg::config::grammar>(), 0); }

TEST(ConfigGrammar, HEX) {
  auto checkHex = [](const std::string& input) {
    std::optional<flexi_cfg::config::ActionData> out;
    checkResult<peg::must<flexi_cfg::config::HEX, peg::eolf>,
                flexi_cfg::config::types::ConfigValue>(
        input, flexi_cfg::config::types::Type::kNumber, out);
  };
  {
    const std::string content = "0x0";
    checkHex(content);
  }
  {
    const std::string content = "0x0de34";
    checkHex(content);
  }
  {
    const std::string content = "0xD34F";
    checkHex(content);
  }
  {
    const std::string content = "0xd0D";
    checkHex(content);
  }
  {
    const std::string content = "0Xd0D0";
    checkHex(content);
  }
  {
    // This will fail due to an extra leading 0
    const std::string content = "00x00";
    std::optional<RetType> ret;
    EXPECT_THROW(ret.emplace(runTest<peg::must<flexi_cfg::config::HEX, peg::eolf>>(content)),
                 std::exception);
  }
  {
    // This will fail due to an alpha character not within the hexadecimal range
    const std::string content = "0xG";
    std::optional<RetType> ret;
    EXPECT_THROW(ret.emplace(runTest<peg::must<flexi_cfg::config::HEX, peg::eolf>>(content)),
                 std::exception);
  }
  {
    // This will fail due to a leading negative sign
    const std::string content = "-0xd0D";
    std::optional<RetType> ret;
    EXPECT_THROW(ret.emplace(runTest<peg::must<flexi_cfg::config::HEX, peg::eolf>>(content)),
                 std::exception);
  }
}

TEST(ConfigGrammar, INTEGER) {
  auto checkInt = [](const std::string& input) {
    std::optional<flexi_cfg::config::ActionData> out;
    checkResult<peg::must<flexi_cfg::config::INTEGER, peg::eolf>,
                flexi_cfg::config::types::ConfigValue>(
        input, flexi_cfg::config::types::Type::kNumber, out);
    const auto value = dynamic_pointer_cast<flexi_cfg::config::types::ConfigValue>(out->obj_res);
    ASSERT_NO_THROW(std::any_cast<int>(value->value_any));
    EXPECT_EQ(std::any_cast<int>(value->value_any), std::stoi(input));
  };
  {
    const std::string content = "-1001";
    checkInt(content);
  }
  {
    const std::string content = "0";
    checkInt(content);
  }
  {
    const std::string content = "-0";
    checkInt(content);
  }
  {
    const std::string content = "+0";
    checkInt(content);
  }
  {
    const std::string content = "+1234567890";
    checkInt(content);
  }
  {
    // This should fail due to the leading zero.
    const std::string content = "0123";
    std::optional<RetType> ret;
    EXPECT_THROW(ret.emplace(runTest<peg::must<flexi_cfg::config::INTEGER, peg::eolf>>(content)),
                 std::exception);
  }
  {
    // This should fail due to being a float.
    const std::string content = "12.3";
    std::optional<RetType> ret;
    EXPECT_THROW(ret.emplace(runTest<peg::must<flexi_cfg::config::INTEGER, peg::eolf>>(content)),
                 std::exception);
  }
  {
    // This should fail due to being a float.
    const std::string content = "0.";
    std::optional<RetType> ret;
    EXPECT_THROW(ret.emplace(runTest<peg::must<flexi_cfg::config::INTEGER, peg::eolf>>(content)),
                 std::exception);
  }
}

TEST(ConfigGrammar, FLOAT) {
  auto checkFloat = [](const std::string& input) {
    std::optional<flexi_cfg::config::ActionData> out;
    checkResult<peg::must<flexi_cfg::config::FLOAT, peg::eolf>,
                flexi_cfg::config::types::ConfigValue>(
        input, flexi_cfg::config::types::Type::kNumber, out);
    const auto value = dynamic_pointer_cast<flexi_cfg::config::types::ConfigValue>(out->obj_res);
    ASSERT_NO_THROW(std::any_cast<double>(value->value_any));
    EXPECT_EQ(std::any_cast<double>(value->value_any), std::stod(input));
  };
  {
    const std::string content = "1234.";
    checkFloat(content);
  }
  {
    const std::string content = "-1234.";
    checkFloat(content);
  }
  {
    const std::string content = "+1234.";
    checkFloat(content);
  }
  {
    const std::string content = "1234.56789";
    checkFloat(content);
  }
  {
    const std::string content = "0.123";
    checkFloat(content);
  }
  {
    const std::string content = "-0.123";
    checkFloat(content);
  }
  {
    const std::string content = "+0.123";
    checkFloat(content);
  }
  {
    const std::string content = "1.23e4";
    checkFloat(content);
  }
  {
    const std::string content = "1.23e+4";
    checkFloat(content);
  }
  {
    const std::string content = "1.23e-4";
    checkFloat(content);
  }
  {
    const std::string content = "1.23E-4";
    checkFloat(content);
  }
  {
    const std::string content = "1.23E0";
    checkFloat(content);
  }
  {
    const std::string content = "1e3";
    checkFloat(content);
  }
  {
    // This will fail due to the leading zero.
    const std::string content = "01.23";
    std::optional<RetType> ret;
    EXPECT_THROW(ret.emplace(runTest<peg::must<flexi_cfg::config::FLOAT, peg::eolf>>(content)),
                 std::exception);
  }
  {
    // This will fail due to being an integer
    const std::string content = "123";
    std::optional<RetType> ret;
    EXPECT_THROW(ret.emplace(runTest<peg::must<flexi_cfg::config::FLOAT, peg::eolf>>(content)),
                 std::exception);
  }
  {
    // This will fail due to the decimal valued exponent
    const std::string content = "1.23e1.2";
    std::optional<RetType> ret;
    EXPECT_THROW(ret.emplace(runTest<peg::must<flexi_cfg::config::FLOAT, peg::eolf>>(content)),
                 std::exception);
  }
  {
    // This will fail due to the exponent value missing.
    const std::string content = "1.23e";
    std::optional<RetType> ret;
    EXPECT_THROW(ret.emplace(runTest<peg::must<flexi_cfg::config::FLOAT, peg::eolf>>(content)),
                 std::exception);
  }
}

TEST(ConfigGrammar, NUMBER) {
  auto checkNumber = [](const std::string& input) {
    std::optional<flexi_cfg::config::ActionData> out;
    checkResult<peg::must<flexi_cfg::config::NUMBER, peg::eolf>,
                flexi_cfg::config::types::ConfigValue>(
        input, flexi_cfg::config::types::Type::kNumber, out);
  };
  // NOTE: Testing of FLOAT and INTEGER is already covered. This just checks that the grammar
  // handles both types.
  {
    const std::string content = "+0.123";
    checkNumber(content);
  }
  {
    const std::string content = "-1.23e4";
    checkNumber(content);
  }
  {
    const std::string content = "1.23e+4";
    checkNumber(content);
  }
  {
    const std::string content = "321";
    checkNumber(content);
  }
  {
    const std::string content = "-312";
    checkNumber(content);
  }
  {
    const std::string content = "+231";
    checkNumber(content);
  }
}

TEST(ConfigGrammar, BOOLEAN) {
  auto checkBoolean = [](const std::string& input, bool expected) {
    std::optional<flexi_cfg::config::ActionData> out;
    checkResult<peg::must<flexi_cfg::config::BOOLEAN, peg::eolf>,
                flexi_cfg::config::types::ConfigValue>(
        input, flexi_cfg::config::types::Type::kBoolean, out);
    const auto value = dynamic_pointer_cast<flexi_cfg::config::types::ConfigValue>(out->obj_res);
    ASSERT_NE(value, nullptr);
    ASSERT_NO_THROW(std::any_cast<bool>(value->value_any));
    EXPECT_EQ(std::any_cast<bool>(value->value_any), expected);
  };

  {
    const std::string content = "true";
    checkBoolean(content, true);
  }
  {
    const std::string content = "false";
    checkBoolean(content, false);
  }

  {
    // This should fail due to the boolean being quoted (it is a string)
    const std::string content = R"("true")";
    std::optional<RetType> ret;
    EXPECT_THROW(ret.emplace(runTest<peg::must<flexi_cfg::config::BOOLEAN, peg::eolf>>(content)),
                 std::exception);
  }
  {
    // This should fail due to incorrect case
    const std::string content = "True";
    std::optional<RetType> ret;
    EXPECT_THROW(ret.emplace(runTest<peg::must<flexi_cfg::config::BOOLEAN, peg::eolf>>(content)),
                 std::exception);
  }
  {
    // This should fail due to incorrect case
    const std::string content = "False";
    std::optional<RetType> ret;
    EXPECT_THROW(ret.emplace(runTest<peg::must<flexi_cfg::config::BOOLEAN, peg::eolf>>(content)),
                 std::exception);
  }
}

TEST(ConfigGrammar, STRING) {
  auto checkString = [](const std::string& input) {
    std::optional<flexi_cfg::config::ActionData> out;
    checkResult<peg::must<flexi_cfg::config::STRING, peg::eolf>,
                flexi_cfg::config::types::ConfigValue>(
        input, flexi_cfg::config::types::Type::kString, out);
    const auto value = dynamic_pointer_cast<flexi_cfg::config::types::ConfigValue>(out->obj_res);
    EXPECT_EQ(value->value, input);
  };
  // A STRING can contain almost anything (except double quotes). Hard to test all possibilities so
  // pick some likely strings along with some expected failures.
  {
    const std::string content = R"("test")";
    checkString(content);
  }
  {
    const std::string content = R"("test with spaces")";
    checkString(content);
  }
  {
    const std::string content = R"("test.with.dots")";
    checkString(content);
  }
  {
    const std::string content = "\"$test\"";
    checkString(content);
  }
  {
    const std::string content = "\"${test}\"";
    checkString(content);
  }
  {
    // This will fail due to a lack of trailing quote
    const std::string content = "\"test";
    std::optional<RetType> ret;
    EXPECT_THROW(ret.emplace(runTest<peg::must<flexi_cfg::config::STRING, peg::eolf>>(content)),
                 std::exception);
  }
  {
    // This will fail due to a lack of leading quote
    const std::string content = "test\"";
    std::optional<RetType> ret;
    EXPECT_THROW(ret.emplace(runTest<peg::must<flexi_cfg::config::STRING, peg::eolf>>(content)),
                 std::exception);
  }
  {
    // This will fail due to an extra internal quote
    const std::string content = R"("te"st")";
    std::optional<RetType> ret;
    EXPECT_THROW(ret.emplace(runTest<peg::must<flexi_cfg::config::STRING, peg::eolf>>(content)),
                 std::exception);
  }
}

const std::vector<std::string> list_test_cases = {
    // Verify that a list of integers is supported
    "[1, 2, 3]",
    // Verify that a list of floats is supported
    "[1.0, 2., -3.3]",
    // Verify that a list of strings is supported
    R"(["one", "two", "three"])",
    // Verify that a list of hex values is supported
    "[0x123, 0Xabc, 0xA1B2F9]",
    // Verify that a list of floats and a variable reference is supported
    "[0.123, $(ref.var), 3.456]",
    // Verify that expressions in lists is supported
    R"([12, {{ 2^14 - 1}}, 0.32])",
    // Verify that lists can contain newlines
    R"([1,
      2,
      3])",
    // Verify that a list can contain leading and trailing comments:
    R"([# comment
      1, 2,   3   # comment
      # comment
      ])",
    "[]",  // Empty lists are supported
    // Verify that an empty list containing a comment is supported
    R"([
      # This is a multi-line
      # comment
      ])",
    // Verify that a list of variables is supported
    "[$(ref.var2), $(ref.var1), 3.456]",
    // Verify that a mix of variables, expressions and numbers is supported
    R"([$(ref.var2), {{ 2^14 - 1}}, 0.32])",
    };

TEST(ConfigGrammar, LIST) {
  auto checkList = [](const std::string& input) {
    std::optional<flexi_cfg::config::ActionData> out;
    checkResult<peg::must<flexi_cfg::config::LIST, peg::eolf>,
                flexi_cfg::config::types::ConfigList>(input, flexi_cfg::config::types::Type::kList,
                                                      out);
  };
  for (const auto& content : list_test_cases) {
    checkList(content);
  }

  {
    // Non-homogeneous lists are not allowed
    const std::string content = R"([12, "two", 10.2])";
    std::optional<RetType> ret;
    EXPECT_THROW(ret.emplace(runTest<peg::must<flexi_cfg::config::LIST, peg::eolf>>(content)),
                 flexi_cfg::config::InvalidTypeException);
  }
  {
    // Another form of non-homogeneous list that is not allowed.
    // An expression always evaluates to a number, so this list is not allowed because it would mix
    // numbers and strings.
    const std::string content = R"(["TWO", {{ pi }}, "0.32"])";
    std::optional<RetType> ret;
    EXPECT_THROW(ret.emplace(runTest<peg::must<flexi_cfg::config::LIST, peg::eolf>>(content)),
                 flexi_cfg::config::InvalidTypeException);
  }
  {
    // Fails due to the trailing comma
    const std::string content = "[0x123, 0Xabc, 0xA1B2F9,]";
    std::optional<RetType> ret;
    EXPECT_THROW(ret.emplace(runTest<peg::must<flexi_cfg::config::LIST, peg::eolf>>(content)),
                 tao::pegtl::parse_error);
  }
  {
    // Fails due to containing a VAR
    const std::string content = "[0x123, $VAR, 0xA1B2F9,]";
    std::optional<RetType> ret;
    EXPECT_THROW(ret.emplace(runTest<peg::must<flexi_cfg::config::LIST, peg::eolf>>(content)),
                 tao::pegtl::parse_error);
  }
}

TEST(ConfigGrammar, VALUE) {
  // NOTE: Can't use the `checkResult` helper here due to the need to check multiple types.
  auto checkValue = [](const std::string& input) {
    std::optional<RetType> ret;
    ASSERT_NO_THROW(ret.emplace(runTest<peg::must<flexi_cfg::config::VALUE, peg::eolf>>(input)));
    ASSERT_TRUE(ret.has_value());
    ASSERT_TRUE(ret.value().first);
    auto isValueType = [](const std::shared_ptr<flexi_cfg::config::types::ConfigBase>& in) {
      return in->type == flexi_cfg::config::types::Type::kNumber ||
             in->type == flexi_cfg::config::types::Type::kString ||
             in->type == flexi_cfg::config::types::Type::kValue;
    };
    EXPECT_TRUE(isValueType(ret.value().second.obj_res));
    const auto value =
        dynamic_pointer_cast<flexi_cfg::config::types::ConfigValue>(ret.value().second.obj_res);
    EXPECT_NE(value, nullptr);
  };
  {
    const std::string content = "0x0ab0";
    checkValue(content);
  }
  {
    const std::string content = "-1245";
    checkValue(content);
  }
  {
    const std::string content = "+1.23E-48";
    checkValue(content);
  }
  {
    const std::string content = "\"This is a string\"";
    checkValue(content);
  }
}

TEST(ConfigGrammar, KEY) {
  auto checkKey = [](const std::string& input) {
    std::optional<RetType> ret;
    ASSERT_NO_THROW(ret.emplace(runTest<peg::must<flexi_cfg::config::KEY, peg::eolf>>(input)));
    ASSERT_TRUE(ret.has_value());
    ASSERT_TRUE(ret.value().first);
    EXPECT_EQ(ret.value().second.keys.size(), 1);
    EXPECT_EQ(ret.value().second.flat_keys.size(), 0);
    EXPECT_EQ(input, ret.value().second.keys[0]);
  };

  auto failKey = [](const std::string& input) {
    std::optional<RetType> ret;
    EXPECT_THROW(ret.emplace(runTest<peg::must<flexi_cfg::config::KEY, peg::eolf>>(input)),
                 std::exception)
        << "Input key: " << input;
  };

  {
    // All valid keys:
    const std::vector<std::string> valid = {
        "key", "key2", "k_ey2", "key_2", "kEy2", "kEy2_", "really_long_key_that_has_numbers12_329"};
    for (const auto& content : valid) {
      checkKey(content);
    }
  }
  {
    // This will fail due to starting with a capital letter
    const std::string content = "Key";
    failKey(content);
  }
  {
    // This will fail due to starting with a number
    const std::string content = "1key";
    failKey(content);
  }
  {
    // This will fail due to starting with an underscore
    const std::string content = "_key";
    failKey(content);
  }
  {
    // These will fail due to containing invalid characters
    const std::vector<std::string> invalid = {"ke&y", "k%ey", "^key", "key!", "ke#y"};
    for (const auto& content : invalid) {
      failKey(content);
    }
  }
  {
    // These will fail due to being reserved keywords
    const std::vector<std::string> invalid = {"struct", "proto", "reference", "as"};
    for (const auto& content : invalid) {
      failKey(content);
    }
  }
  {
    // Keys can start with reserved keywords (or contain them) and be valid:
    const std::vector<std::string> valid = {"struct_", "proto_", "my_reference", "spas", "endgame"};
    for (const auto& content : valid) {
      checkKey(content);
    }
  }
  {
    // This will fail (it's a `FLAT_KEY`, not a key)
    const std::string content = "this.is.a.flat.key";
    failKey(content);
  }
}

TEST(ConfigGrammar, FLATKEY) {
  {
    const std::string content = "this.is.a.var.ref";
    auto ret = runTest<peg::must<flexi_cfg::config::FLAT_KEY, peg::eolf>>(content);
    EXPECT_TRUE(ret.first);
    ASSERT_EQ(ret.second.flat_keys.size(), 1);
    ASSERT_EQ(ret.second.flat_keys[0], content);
  }
  {
    const std::string content = "flat_key";
    auto ret = runTest<peg::must<flexi_cfg::config::FLAT_KEY, peg::eolf>>(content);
    EXPECT_TRUE(ret.first);
    ASSERT_EQ(ret.second.flat_keys.size(), 1);
    ASSERT_EQ(ret.second.flat_keys[0], content);
  }
}

TEST(ConfigGrammar, VAR) {
  auto checkVar = [](const std::string& input) {
    std::optional<flexi_cfg::config::ActionData> out;
    checkResult<peg::must<flexi_cfg::config::VAR, peg::eolf>, flexi_cfg::config::types::ConfigVar>(
        input, flexi_cfg::config::types::Type::kVar, out);
    const auto value = dynamic_pointer_cast<flexi_cfg::config::types::ConfigVar>(out->obj_res);
    EXPECT_EQ(value->name, input);
  };
  auto failVar = [](const std::string& input) {
    std::optional<RetType> ret;
    EXPECT_THROW(ret.emplace(runTest<peg::must<flexi_cfg::config::VAR, peg::eolf>>(input)),
                 std::exception);
  };

  {
    // These are all valid vars
    const std::vector<std::string> valid = {"$VAR",   "$V",     "$VAR_",   "$V_",     "$V0",
                                            "$VAR_1", "${VAR}", "${VAR1}", "${VAR_1}"};
    for (const auto& content : valid) {
      checkVar(content);
    }
  }
  {
    // These are invalid vars
    const std::vector<std::string> invalid = {"$",     "VAR",   "$var",  "$VAR$", "$VAR#",  "$Var",
                                              "$1VAR", "$_VAR", "${VAR", "$VAR}", "${_VAR}"};
    for (const auto& content : invalid) {
      failVar(content);
    }
  }
}

TEST(ConfigGrammar, PROTOLIST) {
  auto checkProtoList = [](const std::string& input) {
    std::optional<flexi_cfg::config::ActionData> out;
    checkResult<peg::must<flexi_cfg::config::PROTO_LIST, peg::eolf>,
                flexi_cfg::config::types::ConfigList>(input, flexi_cfg::config::types::Type::kList,
                                                      out);
  };
  // All valid "LIST" cases are also valid "PROTO_LIST" cases
  for (const auto& content : list_test_cases) {
    checkProtoList(content);
  }
  {
    const std::string content = "[3, 4, ${TEST}]";
    checkProtoList(content);
  }
  {
    const std::string content = "[0.35, ${TEST}, -3.14159, $VAR]";
    checkProtoList(content);
  }
  {
    const std::string content = "[0.35, ${TEST}, 0xA4, $VAR]";
    checkProtoList(content);
  }
  {
    const std::string content = "[0.35, 12, 0xA4, -1e+4]";
    checkProtoList(content);
  }
  {
    const std::string content = "[0.35, $(foo.bar), 0xA4, -1e+4]";
    checkProtoList(content);
  }
  {
    const std::string content = "[0.35, $(foo.bar), 0xA4, $TEST]";
    checkProtoList(content);
  }
  {
    // Fails due to non-homogeneous list
    const std::string content = R"([0.35, 12, "fail", -1e+4])";
    std::optional<RetType> ret;
    EXPECT_THROW(ret.emplace(runTest<peg::must<flexi_cfg::config::PROTO_LIST, peg::eolf>>(content)),
                 flexi_cfg::config::InvalidTypeException);
  }
}

TEST(ConfigGrammar, VALUELOOKUP) {
  auto checkVarRef = [](const std::string& input) {
    std::optional<flexi_cfg::config::ActionData> out;
    checkResult<peg::must<flexi_cfg::config::VALUE_LOOKUP, peg::eolf>,
                flexi_cfg::config::types::ConfigValueLookup>(
        input, flexi_cfg::config::types::Type::kValueLookup, out);
    const auto value =
        dynamic_pointer_cast<flexi_cfg::config::types::ConfigValueLookup>(out->obj_res);
    EXPECT_EQ("$(" + value->var() + ")", input);
  };
  auto failVarRef = [](const std::string& input) {
    std::optional<RetType> ret;
    EXPECT_THROW(ret.emplace(runTest<peg::must<flexi_cfg::config::VAR, peg::eolf>>(input)),
                 std::exception);
  };
  // These are all valid vars
  const std::vector<std::string> valid = {
      "$(this.is.a.var.ref)",   "$(single_key)", "$(this.is.a.$VAR.ref)",
      "$($THIS.is.a.var.ref)",  "$($VAR_REF)",   "$(this.is.a.${VAR}.ref)",
      "$($THIS.is.a.var.$REF)", "$($VAR.$REF)"};

  for (const auto& content : valid) {
    checkVarRef(content);
  }
}

TEST(ConfigGrammar, PAIR) {
  // Can't use the `checkResult` helper here.
  auto checkPair = [](const std::string& input, flexi_cfg::config::types::Type expected_type,
                      bool is_override) {
    std::cout << "input: '" << input << "'\n";
    auto ret = runTest<peg::must<flexi_cfg::config::PAIR, peg::eolf>>(input);
    ASSERT_TRUE(ret.first);
    auto& out = ret.second;

    fmt::print("out: {}\n", ret.first);
    out.print(std::cout);
    if (is_override) {
      EXPECT_TRUE(out.is_override);
      EXPECT_TRUE(out.cfg_res.front().empty());
      EXPECT_EQ(out.override_values.size(), 1);
      const auto& [key, value] = *out.override_values.begin();
      EXPECT_EQ(key, "key");
      EXPECT_EQ(value->type, expected_type);
    } else {
      EXPECT_FALSE(out.is_override);
      EXPECT_EQ(out.cfg_res.size(), 1);
      EXPECT_TRUE(out.override_values.empty());
      const auto& cfg_map = out.cfg_res.front();
      EXPECT_EQ(cfg_map.size(), 1);
      const auto& [key, value] = *cfg_map.begin();
      EXPECT_EQ(key, "key");
      EXPECT_EQ(value->type, expected_type);
    }
  };
  using flexi_cfg::config::types::Type;

  {
    constexpr std::string_view content = "key{}= 123";
    checkPair(fmt::format(content, ""), Type::kNumber, false);
    checkPair(fmt::format(content, "  [override]  "), Type::kNumber, true);
  }
  {
    constexpr std::string_view content = "key{}= 0x123";
    checkPair(fmt::format(content, ""), Type::kNumber, false);
    checkPair(fmt::format(content, "[override]"), Type::kNumber, true);
  }
  {
    constexpr std::string_view content = "key{}= 0.123";
    checkPair(fmt::format(content, " "), Type::kNumber, false);
    checkPair(fmt::format(content, "[override] "), Type::kNumber, true);
  }
  {
    // NOTE: Extra {} required due to the usage of `fmt::format`
    constexpr std::string_view content = "key    {}  = {{{{ 2 * pi }}}}";
    checkPair(fmt::format(content, " "), Type::kExpression, false);
    checkPair(fmt::format(content, "[override] "), Type::kExpression, true);
  }
  {
    constexpr std::string_view content = R"(key{}= "value")";
    checkPair(fmt::format(content, "\t"), Type::kString, false);
    checkPair(fmt::format(content, "\t[override]\t"), Type::kString, true);
  }
  {
    constexpr const std::string_view content = "key{}= true";
    checkPair(fmt::format(content, "  "), Type::kBoolean, false);
    checkPair(fmt::format(content, "      [override] "), Type::kBoolean, true);
  }
  {
    constexpr std::string_view content = "key{}= false";
    checkPair(fmt::format(content, " "), Type::kBoolean, false);
    checkPair(fmt::format(content, " [override]   "), Type::kBoolean, true);
  }

  {
    auto checkFail = [](const std::string& input) {
      std::optional<RetType> ret;
      EXPECT_THROW(ret.emplace(runTest<peg::must<flexi_cfg::config::PAIR, peg::eolf>>(input)),
                   std::exception);
    };
    // Newline values are not allowed within a pair. Check this.
    constexpr std::string_view content = "key{}={}0";

    checkFail(fmt::format(content, "\n", ""));
    checkFail(fmt::format(content, "", "\n"));
    checkFail(fmt::format(content, " [override]\n", ""));
    checkFail(fmt::format(content, "\n[override] ", ""));
  }
}

TEST(ConfigGrammar, FULLPAIR) {
  const std::string flat_key = "my.flat.value";

  auto checkFullPair = [&flat_key](const std::string& input, flexi_cfg::config::types::Type expected_type,
                         bool is_override) {
    std::cout << "input: '" << input << "'\n";
    auto ret = runTest<peg::must<flexi_cfg::config::FULLPAIR, peg::eolf>>(input);
    ASSERT_TRUE(ret.first);
    auto& out = ret.second;

    fmt::print("out: {}\n", ret.first);
    out.print(std::cout);

    EXPECT_EQ(out.is_override, is_override);
    if (is_override) {
      EXPECT_TRUE(out.cfg_res.front().empty());

      EXPECT_EQ(out.override_values.size(), 1);
      const auto& [key, value] = *out.override_values.begin();
      EXPECT_EQ(key, flat_key);
      EXPECT_EQ(value->type, expected_type);
    } else {
      // Not an override, so there should be no override values:
      EXPECT_TRUE(out.override_values.empty());

      // Eliminate any vector elements with an empty map. This may be the case due to the way that flat
      // keys are resolved into structs.
      // After this cleanup, there should be exactly one map left.
      out.cfg_res.erase(
          std::remove_if(std::begin(out.cfg_res), std::end(out.cfg_res),
                        [](const auto& m) { return m.empty(); }),
          std::end(out.cfg_res));
      ASSERT_EQ(out.cfg_res.size(), 1);
      // Check the keys that exist in the map and find the leaf node.
      flexi_cfg::config::types::CfgMap* cfg_map = &out.cfg_res.front();
      const auto keys = flexi_cfg::utils::split(flat_key, '.');
      for (const auto& key : keys) {
        ASSERT_TRUE(cfg_map->contains(key));
        auto struct_like =
            dynamic_pointer_cast<flexi_cfg::config::types::ConfigStructLike>(cfg_map->at(key));
        if (struct_like != nullptr) {
          cfg_map = &struct_like->data;
        }
      }
      // There should only be a single leaf node left in the map.
      EXPECT_EQ(cfg_map->size(), 1);
      EXPECT_EQ(cfg_map->at(keys.back())->type, expected_type);
    }
  };
  using flexi_cfg::config::types::Type;

  {
    const auto value = 123;
    checkFullPair(fmt::format("{} = {}", flat_key, value), Type::kNumber, false);
    checkFullPair(fmt::format("{} [override] = {}", flat_key, value), Type::kNumber, true);
  }
  {
    const auto *const value = "0x123";
    checkFullPair(fmt::format("{} = {}", flat_key, value), Type::kNumber, false);
    checkFullPair(fmt::format("{} [override] = {}", flat_key, value), Type::kNumber, true);
  }
  {
    const auto value = 5.37e-3;
    checkFullPair(fmt::format("{} = {}", flat_key, value), Type::kNumber, false);
    checkFullPair(fmt::format("{} [override] = {}", flat_key, value), Type::kNumber, true);
  }
  {
    const auto *const value = "{{ 2 * pi }}";
    checkFullPair(fmt::format("{} = {}", flat_key, value), Type::kExpression, false);
    checkFullPair(fmt::format("{} [override] = {}", flat_key, value), Type::kExpression, true);
  }
  {
    const auto *const value = "value";
    checkFullPair(fmt::format("{} = \"{}\"", flat_key, value), Type::kString, false);
    checkFullPair(fmt::format("{} [override] = \"{}\"", flat_key, value), Type::kString, true);
  }
  {
    checkFullPair(fmt::format("{} = true", flat_key), Type::kBoolean, false);
    checkFullPair(fmt::format("{} [override] = true", flat_key), Type::kBoolean, true);
  }
}

TEST(ConfigGrammar, INCLUDE_ABSOLUTE) {
  const std::filesystem::path tmp_path = fmt::format("/tmp/flexi_cfg/{}_absolute.cfg", getpid());
  create_directories(tmp_path.parent_path());
  const auto* const cfg_content = "\n  foo = 123 \n";
  std::ofstream absolute_cfg_file(tmp_path, std::ios::out | std::ios::trunc);
  absolute_cfg_file << cfg_content << std::flush;
  const std::string test_config = fmt::format("include {}", tmp_path.string());
  auto ret = runTest<peg::must<flexi_cfg::config::INCLUDE>>(test_config);
  std::ignore = std::filesystem::remove(tmp_path);
  EXPECT_TRUE(ret.first);
  ASSERT_EQ(ret.second.cfg_res.size(), 1);
  auto* cfg_map = &ret.second.cfg_res.front();
  ASSERT_TRUE(cfg_map->contains("foo"));
  auto val = std::dynamic_pointer_cast<flexi_cfg::config::types::ConfigValue>(cfg_map->at("foo"));
  ASSERT_EQ(val.get()->value, "123");
}

TEST(ConfigGrammar, INCLUDE) {
  const std::string test_config = "include nested/simple_include.cfg";
  auto ret = runTest<peg::must<flexi_cfg::config::INCLUDE>>(test_config);
  EXPECT_TRUE(ret.first);
  ASSERT_EQ(ret.second.cfg_res.size(), 1);
  auto* cfg_map = &ret.second.cfg_res.front();
  ASSERT_TRUE(cfg_map->contains("foo"));
  auto val = std::dynamic_pointer_cast<flexi_cfg::config::types::ConfigValue>(cfg_map->at("foo"));
  ASSERT_EQ(val.get()->value, "123");
}

TEST(ConfigGrammar, INCLUDE_OPTIONAL) {
  const std::string test_config =
      "include nested/simple_include.cfg\n"
      "include [optional] nested/does_not_exist.cfg\n";
  auto ret = runTest<peg::must<flexi_cfg::config::includes>>(test_config);

  EXPECT_TRUE(ret.first);
  ASSERT_EQ(ret.second.cfg_res.size(), 1);
  auto* cfg_map = &ret.second.cfg_res.front();
  ASSERT_TRUE(cfg_map->contains("foo"));
  auto val = std::dynamic_pointer_cast<flexi_cfg::config::types::ConfigValue>(cfg_map->at("foo"));
  ASSERT_EQ(val.get()->value, "123");
}

TEST(ConfigGrammar, INCLUDE_ONCE) {
  flexi_cfg::logger::Logger::instance().setLevel(flexi_cfg::logger::Severity::TRACE);
  const std::string test_config =
      "include nested/simple_include.cfg\n"
      "include [once] nested/simple_include.cfg\n";  // without [once] it shouldn't work
  auto ret = runTest<peg::must<flexi_cfg::config::includes>>(test_config);

  EXPECT_TRUE(ret.first);
  ASSERT_EQ(ret.second.cfg_res.size(), 1);
  auto* cfg_map = &ret.second.cfg_res.front();
  ASSERT_TRUE(cfg_map->contains("foo"));
  auto val = std::dynamic_pointer_cast<flexi_cfg::config::types::ConfigValue>(cfg_map->at("foo"));
  ASSERT_EQ(val.get()->value, "123");
}

TEST(ConfigGrammar, INCLUDE_ONCE_OPTIONAL) {
  flexi_cfg::logger::Logger::instance().setLevel(flexi_cfg::logger::Severity::TRACE);
  const std::string test_config =
      "include nested/simple_include.cfg\n"
      "include [once][optional][optional]  nested/simple_include.cfg\n";  // without [once] it
                                                                          // shouldn't work
  auto ret = runTest<peg::must<flexi_cfg::config::includes>>(test_config);

  EXPECT_TRUE(ret.first);
  ASSERT_EQ(ret.second.cfg_res.size(), 1);
  auto* cfg_map = &ret.second.cfg_res.front();
  ASSERT_TRUE(cfg_map->contains("foo"));
  auto val = std::dynamic_pointer_cast<flexi_cfg::config::types::ConfigValue>(cfg_map->at("foo"));
  ASSERT_EQ(val.get()->value, "123");
}

TEST(ConfigGrammar, INCLUDE_RELATIVE) {
  const std::string test_config = "include_relative nested/simple_include.cfg\n";
  auto ret = runTest<peg::must<flexi_cfg::config::includes>>(test_config);

  EXPECT_TRUE(ret.first);
  ASSERT_EQ(ret.second.cfg_res.size(), 1);
  auto* cfg_map = &ret.second.cfg_res.front();
  ASSERT_TRUE(cfg_map->contains("foo"));
  auto val = std::dynamic_pointer_cast<flexi_cfg::config::types::ConfigValue>(cfg_map->at("foo"));
  ASSERT_EQ(val.get()->value, "123");
}

TEST(ConfigGrammar, INCLUDE_RELATIVE_OPTIONAL) {
  const std::string test_config =
      "include_relative nested/simple_include.cfg\n"
      "include_relative [optional] nested/does_not_exist.cfg\n\n";
  auto ret = runTest<peg::must<flexi_cfg::config::includes>>(test_config);

  EXPECT_TRUE(ret.first);
  ASSERT_EQ(ret.second.cfg_res.size(), 1);
  auto* cfg_map = &ret.second.cfg_res.front();
  ASSERT_TRUE(cfg_map->contains("foo"));
  auto val = std::dynamic_pointer_cast<flexi_cfg::config::types::ConfigValue>(cfg_map->at("foo"));
  ASSERT_EQ(val.get()->value, "123");
}

// NOLINTEND(google-readability-avoid-underscore-in-googletest-name)

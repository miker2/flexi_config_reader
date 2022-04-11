#include "config_grammar.h"

#include <gtest/gtest.h>

#include <optional>
#include <tao/pegtl/contrib/analyze.hpp>

#include "config_actions.h"
#include "config_classes.h"

namespace peg = TAO_PEGTL_NAMESPACE;

namespace {
using RetType = std::pair<bool, config::ActionData>;

template <typename GTYPE>
auto runTest(const std::string& test_str) -> RetType {
  peg::memory_input in(test_str, "from_content");

  config::ActionData out;
  const auto ret = peg::parse<GTYPE, config::action>(in, out);
  // out.print();
  return {ret, out};
}

template <typename GRAMMAR, typename T>
void checkResult(const std::string& input, config::types::Type expected_type,
                 std::optional<config::ActionData>& out) {
  std::optional<RetType> ret;
  ASSERT_NO_THROW(ret.emplace(runTest<GRAMMAR>(input)));
  ASSERT_TRUE(ret.has_value());
  ASSERT_TRUE(ret.value().first);
  EXPECT_EQ(ret.value().second.obj_res->type, expected_type);
  const auto value = dynamic_pointer_cast<T>(ret.value().second.obj_res);
  EXPECT_NE(value, nullptr);

  if (ret.has_value()) {
    out = ret.value().second;
  }
}
}  // namespace

TEST(config_grammar, analyze) { ASSERT_EQ(peg::analyze<config::grammar>(), 0); }

TEST(config_grammar, INTEGER) {
  auto checkInt = [](const std::string& input) {
    std::optional<config::ActionData> out;
    checkResult<peg::must<config::INTEGER, peg::eolf>, config::types::ConfigValue>(
        input, config::types::Type::kNumber, out);
    const auto value = dynamic_pointer_cast<config::types::ConfigValue>(out->obj_res);
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
    EXPECT_THROW(ret.emplace(runTest<peg::must<config::INTEGER, peg::eolf>>(content)),
                 std::exception);
  }
  {
    // This should fail due to being a float.
    const std::string content = "12.3";
    std::optional<RetType> ret;
    EXPECT_THROW(ret.emplace(runTest<peg::must<config::INTEGER, peg::eolf>>(content)),
                 std::exception);
  }
  {
    // This should fail due to being a float.
    const std::string content = "0.";
    std::optional<RetType> ret;
    EXPECT_THROW(ret.emplace(runTest<peg::must<config::INTEGER, peg::eolf>>(content)),
                 std::exception);
  }
}

TEST(config_grammar, FLOAT) {
  auto checkFloat = [](const std::string& input) {
    std::optional<config::ActionData> out;
    checkResult<peg::must<config::FLOAT, peg::eolf>, config::types::ConfigValue>(
        input, config::types::Type::kNumber, out);
    const auto value = dynamic_pointer_cast<config::types::ConfigValue>(out->obj_res);
    ASSERT_NO_THROW(std::any_cast<double>(value->value_any));
    EXPECT_EQ(std::any_cast<double>(value->value_any), std::stod(input));
  };
  {
    std::string content = "1234.";
    checkFloat(content);
  }
  {
    std::string content = "-1234.";
    checkFloat(content);
  }
  {
    std::string content = "+1234.";
    checkFloat(content);
  }
  {
    std::string content = "1234.56789";
    checkFloat(content);
  }
  {
    std::string content = "0.123";
    checkFloat(content);
  }
  {
    std::string content = "-0.123";
    checkFloat(content);
  }
  {
    std::string content = "+0.123";
    checkFloat(content);
  }
  {
    std::string content = "1.23e4";
    checkFloat(content);
  }
  {
    std::string content = "1.23e+4";
    checkFloat(content);
  }
  {
    std::string content = "1.23e-4";
    checkFloat(content);
  }
  {
    std::string content = "1.23E-4";
    checkFloat(content);
  }
  {
    std::string content = "1.23E0";
    checkFloat(content);
  }
  {
    // This will fail due to the leading zero.
    std::string content = "01.23";
    std::optional<RetType> ret;
    EXPECT_THROW(ret.emplace(runTest<peg::must<config::FLOAT, peg::eolf>>(content)),
                 std::exception);
  }
  {
    // This will fail due to being an integer
    std::string content = "123";
    std::optional<RetType> ret;
    EXPECT_THROW(ret.emplace(runTest<peg::must<config::FLOAT, peg::eolf>>(content)),
                 std::exception);
  }
  {
    // This will fail due to the decimal valued exponent
    std::string content = "1.23e1.2";
    std::optional<RetType> ret;
    EXPECT_THROW(ret.emplace(runTest<peg::must<config::FLOAT, peg::eolf>>(content)),
                 std::exception);
  }
  {
    // This will fail due to the exponent value missing.
    std::string content = "1.23e";
    std::optional<RetType> ret;
    EXPECT_THROW(ret.emplace(runTest<peg::must<config::FLOAT, peg::eolf>>(content)),
                 std::exception);
  }
}

TEST(config_grammar, NUMBER) {
  auto checkNumber = [](const std::string& input) {
    std::optional<config::ActionData> out;
    checkResult<peg::must<config::NUMBER, peg::eolf>, config::types::ConfigValue>(
        input, config::types::Type::kNumber, out);
  };
  // NOTE: Testing of FLOAT and INTEGER is already covered. This just checks that the grammar
  // handles both types.
  {
    std::string content = "+0.123";
    checkNumber(content);
  }
  {
    std::string content = "-1.23e4";
    checkNumber(content);
  }
  {
    std::string content = "1.23e+4";
    checkNumber(content);
  }
  {
    std::string content = "321";
    checkNumber(content);
  }
  {
    std::string content = "-312";
    checkNumber(content);
  }
  {
    std::string content = "+231";
    checkNumber(content);
  }
}

TEST(config_grammar, HEX) {
  auto checkHex = [](const std::string& input) {
    std::optional<config::ActionData> out;
    checkResult<peg::must<config::HEX, peg::eolf>, config::types::ConfigValue>(
        input, config::types::Type::kNumber, out);
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
    std::string content = "00x00";
    std::optional<RetType> ret;
    EXPECT_THROW(ret.emplace(runTest<peg::must<config::HEX, peg::eolf>>(content)), std::exception);
  }
  {
    // This will fail due to an alpha character not within the hexadecimal range
    std::string content = "0xG";
    std::optional<RetType> ret;
    EXPECT_THROW(ret.emplace(runTest<peg::must<config::HEX, peg::eolf>>(content)), std::exception);
  }
  {
    // This will fail due to a leading negative sign
    std::string content = "-0xd0D";
    std::optional<RetType> ret;
    EXPECT_THROW(ret.emplace(runTest<peg::must<config::HEX, peg::eolf>>(content)), std::exception);
  }
}

TEST(config_grammar, STRING) {
  auto checkString = [](const std::string& input) {
    std::optional<config::ActionData> out;
    checkResult<peg::must<config::STRING, peg::eolf>, config::types::ConfigValue>(
        input, config::types::Type::kString, out);
    const auto value = dynamic_pointer_cast<config::types::ConfigValue>(out->obj_res);
    EXPECT_EQ(value->value, input);
  };
  // A STRING can contain almost anything (except double quotes). Hard to test all possibilities so
  // pick some likely strings along with some expected failures.
  {
    const std::string content = "\"test\"";
    checkString(content);
  }
  {
    const std::string content = "\"test with spaces\"";
    checkString(content);
  }
  {
    const std::string content = "\"test.with.dots\"";
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
    EXPECT_THROW(ret.emplace(runTest<peg::must<config::STRING, peg::eolf>>(content)),
                 std::exception);
  }
  {
    // This will fail due to a lack of leading quote
    const std::string content = "test\"";
    std::optional<RetType> ret;
    EXPECT_THROW(ret.emplace(runTest<peg::must<config::STRING, peg::eolf>>(content)),
                 std::exception);
  }
  {
    // This will fail due to an extra internal quote
    const std::string content = "\"te\"st\"";
    std::optional<RetType> ret;
    EXPECT_THROW(ret.emplace(runTest<peg::must<config::STRING, peg::eolf>>(content)),
                 std::exception);
  }
}

TEST(config_grammar, VALUE) {
  // NOTE: Can't use the `checkResult` helper here due to the need to check multiple types.
  auto checkValue = [](const std::string& input) {
    std::optional<RetType> ret;
    ASSERT_NO_THROW(ret.emplace(runTest<peg::must<config::VALUE, peg::eolf>>(input)));
    ASSERT_TRUE(ret.has_value());
    ASSERT_TRUE(ret.value().first);
    auto isValueType = [](const std::shared_ptr<config::types::ConfigBase>& in) {
      return in->type == config::types::Type::kNumber || in->type == config::types::Type::kString ||
             in->type == config::types::Type::kValue;
    };
    EXPECT_TRUE(isValueType(ret.value().second.obj_res));
    const auto value = dynamic_pointer_cast<config::types::ConfigValue>(ret.value().second.obj_res);
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

TEST(config_grammar, FULLPAIR) {
  const std::string flat_key = "float.my.value";
  std::string content = flat_key + "   =  5.37e+6";

  auto ret = runTest<peg::must<config::FULLPAIR, peg::eolf>>(content);
  EXPECT_TRUE(ret.first);
  ASSERT_EQ(ret.second.cfg_res.size(), 1);
  ASSERT_TRUE(ret.second.cfg_res.front().contains(flat_key));
  EXPECT_EQ(ret.second.cfg_res.front()[flat_key]->type, config::types::Type::kNumber);
}

TEST(config_grammar, FLAT_KEY) {
  std::string content = "this.is.a.var.ref";
  auto ret = runTest<peg::must<config::FLAT_KEY, peg::eolf>>(content);
  EXPECT_TRUE(ret.first);
  ASSERT_EQ(ret.second.flat_keys.size(), 1);
  ASSERT_EQ(ret.second.flat_keys[0], content);
}

TEST(config_grammar, VAR_REF) {
  std::string content = "$(this.is.a.var.ref)";

  std::optional<config::ActionData> out;
  checkResult<peg::must<config::VAR_REF, peg::eolf>, config::types::ConfigValueLookup>(
      content, config::types::Type::kValueLookup, out);
}

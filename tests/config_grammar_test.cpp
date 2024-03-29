#include <gtest/gtest.h>

#include <algorithm>
#include <optional>
#include <tao/pegtl/contrib/analyze.hpp>

#include "flexi_cfg/config/actions.h"
#include "flexi_cfg/config/classes.h"
#include "flexi_cfg/config/grammar.h"

namespace peg = TAO_PEGTL_NAMESPACE;

namespace {
using RetType = std::pair<bool, flexi_cfg::config::ActionData>;

template <typename GTYPE>
auto runTest(const std::string& test_str) -> RetType {
  peg::memory_input in(test_str, "from_content");

  flexi_cfg::config::ActionData out;
  const auto ret = peg::parse<GTYPE, flexi_cfg::config::action>(in, out);
  // out.print();
  return {ret, out};
}

template <typename GRAMMAR, typename T>
void checkResult(const std::string& input, flexi_cfg::config::types::Type expected_type,
                 std::optional<flexi_cfg::config::ActionData>& out) {
  std::optional<RetType> ret;
  ASSERT_NO_THROW(ret.emplace(runTest<GRAMMAR>(input)));
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

TEST(ConfigGrammar, LIST) {
  auto checkList = [](const std::string& input) {
    std::optional<flexi_cfg::config::ActionData> out;
    checkResult<peg::must<flexi_cfg::config::LIST, peg::eolf>,
                flexi_cfg::config::types::ConfigList>(input, flexi_cfg::config::types::Type::kList,
                                                      out);
  };
  {
    const std::string content = "[1, 2, 3]";
    checkList(content);
  }
  {
    const std::string content = "[1.0, 2., -3.3]";
    checkList(content);
  }
  {
    const std::string content = R"(["one", "two", "three"])";
    checkList(content);
  }
  {
    const std::string content = "[0x123, 0Xabc, 0xA1B2F9]";
    checkList(content);
  }
  {
    const std::string content = "[0.123, $(ref.var), 3.456]";
    checkList(content);
  }
  {
    const std::string content = "[$(ref.var2), $(ref.var1), 3.456]";
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
    out->print(std::cout);
  };
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

TEST(ConfigGrammar, FULLPAIR) {
  const std::string flat_key = "float.my.value";
  const std::string content = flat_key + "   =  5.37e+6";

  auto ret = runTest<peg::must<flexi_cfg::config::FULLPAIR, peg::eolf>>(content);
  EXPECT_TRUE(ret.first);
  // Eliminate any vector elements with an empty map. This may be the case due to the way that flat
  // keys are resolved into structs.
  ret.second.cfg_res.erase(
      std::remove_if(std::begin(ret.second.cfg_res), std::end(ret.second.cfg_res),
                     [](const auto& m) { return m.empty(); }),
      std::end(ret.second.cfg_res));
  ASSERT_EQ(ret.second.cfg_res.size(), 1);
  flexi_cfg::config::types::CfgMap* cfg_map = &ret.second.cfg_res.front();
  const auto keys = flexi_cfg::utils::split(flat_key, '.');
  for (const auto& key : keys) {
    ASSERT_TRUE(cfg_map->contains(key));
    auto struct_like =
        dynamic_pointer_cast<flexi_cfg::config::types::ConfigStructLike>(cfg_map->at(key));
    if (struct_like != nullptr) {
      cfg_map = &struct_like->data;
    }
  }
  EXPECT_EQ(cfg_map->at(keys.back())->type, flexi_cfg::config::types::Type::kNumber);
}

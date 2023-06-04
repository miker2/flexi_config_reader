#include <gtest/gtest.h>

#include <magic_enum.hpp>

#include "flexi_cfg/config/classes.h"
#include "flexi_cfg/config/exceptions.h"
#include "flexi_cfg/config/helpers.h"
#include "flexi_cfg/logger.h"

namespace {
template <typename T, typename... Args>
void testIsStructLike(Args&&... args) {
  const auto obj = std::make_shared<T>(args...);
  EXPECT_EQ(flexi_cfg::config::helpers::isStructLike(obj),
            dynamic_pointer_cast<flexi_cfg::config::types::ConfigStructLike>(obj) != nullptr);
}
}  // namespace

#if defined(__APPLE__) && __clang_major__ < 14
namespace {
constexpr auto kValue = flexi_cfg::config::types::Type::kValue;
}
#else
using flexi_cfg::config::types::Type::kValue;
#endif

TEST(ConfigHelpers, isStructLike) {
  testIsStructLike<flexi_cfg::config::types::ConfigValue>("", kValue);

  testIsStructLike<flexi_cfg::config::types::ConfigValueLookup>("");

  testIsStructLike<flexi_cfg::config::types::ConfigVar>("");

  testIsStructLike<flexi_cfg::config::types::ConfigStruct>("struct", 0);

  testIsStructLike<flexi_cfg::config::types::ConfigProto>("proto", 0);

  testIsStructLike<flexi_cfg::config::types::ConfigReference>("reference", "proto", 0);
}

TEST(ConfigHelpers, checkForErrors) {
  {
    // This test should fail due to duplicate keys
    const std::string key = "key1";
    flexi_cfg::config::types::CfgMap cfg1 = {
        {key, std::make_shared<flexi_cfg::config::types::ConfigValue>(
                  "13", flexi_cfg::config::types::Type::kNumber)}};
    flexi_cfg::config::types::CfgMap cfg2 = {
        {key, std::make_shared<flexi_cfg::config::types::ConfigValue>(
                  "string", flexi_cfg::config::types::Type::kString)}};

    EXPECT_THROW(flexi_cfg::config::helpers::checkForErrors(cfg1, cfg2, key),
                 flexi_cfg::config::DuplicateKeyException);
  }
  {
    // This test should fail due to mismatched types (1 struct-like, the other not)
    const std::string key = "key1";
    flexi_cfg::config::types::CfgMap cfg1 = {
        {key, std::make_shared<flexi_cfg::config::types::ConfigValue>(
                  "13", flexi_cfg::config::types::Type::kNumber)}};
    flexi_cfg::config::types::CfgMap cfg2 = {
        {key, std::make_shared<flexi_cfg::config::types::ConfigStruct>(
                  key, 0 /* depth doesn't matter */)}};

    EXPECT_THROW(flexi_cfg::config::helpers::checkForErrors(cfg1, cfg2, key),
                 flexi_cfg::config::MismatchKeyException);
  }
  {
    // This test should fail due to mismatched types (both struct-like, but different types)
    const std::string key = "key1";
    flexi_cfg::config::types::CfgMap cfg1 = {
        {key, std::make_shared<flexi_cfg::config::types::ConfigProto>(
                  key, 0 /* depth doesn't matter */)}};
    flexi_cfg::config::types::CfgMap cfg2 = {
        {key, std::make_shared<flexi_cfg::config::types::ConfigStruct>(
                  key, 0 /* depth doesn't matter */)}};

    EXPECT_THROW(flexi_cfg::config::helpers::checkForErrors(cfg1, cfg2, key),
                 flexi_cfg::config::MismatchTypeException);
  }
  {
    // This test should be successful
    const std::string key = "key1";
    flexi_cfg::config::types::CfgMap cfg1 = {
        {key, std::make_shared<flexi_cfg::config::types::ConfigStruct>(
                  key, 0 /* depth doesn't matter */)}};
    flexi_cfg::config::types::CfgMap cfg2 = {
        {key, std::make_shared<flexi_cfg::config::types::ConfigStruct>(
                  key, 0 /* depth doesn't matter */)}};

    EXPECT_NO_THROW(flexi_cfg::config::helpers::checkForErrors(cfg1, cfg2, key));
  }
  {
    // This test should fail (no common keys)
    const std::string key1 = "key1";
    flexi_cfg::config::types::CfgMap cfg1 = {
        {key1, std::make_shared<flexi_cfg::config::types::ConfigStruct>(
                   key1, 0 /* depth doesn't matter */)}};

    const std::string key2 = "key2";
    flexi_cfg::config::types::CfgMap cfg2 = {
        {key2, std::make_shared<flexi_cfg::config::types::ConfigStruct>(
                   key2, 0 /* depth doesn't matter */)}};

    EXPECT_THROW(flexi_cfg::config::helpers::checkForErrors(cfg1, cfg2, key1), std::out_of_range)
        << "cfg1: " << cfg1 << ", cfg2: " << cfg2;
    EXPECT_THROW(flexi_cfg::config::helpers::checkForErrors(cfg1, cfg2, key2), std::out_of_range)
        << "cfg1: " << cfg1 << ", cfg2: " << cfg2;
  }
}

TEST(ConfigHelpers, mergeNestedMaps) {
  {
    // This test should succeed (no exceptions thrown)
    const std::string key = "key";
    const std::vector<std::string> inner_keys = {"key1", "key2", "key3", "key4"};
    // NOTE: The value of the keys below don't matter for this test, so just use the empty string

    // cfg1 is:
    //  struct key
    //    key1 = ""
    //    key2 = ""
    flexi_cfg::config::types::CfgMap cfg1_inner = {
        {inner_keys[0], std::make_shared<flexi_cfg::config::types::ConfigValue>("", kValue)},
        {inner_keys[1], std::make_shared<flexi_cfg::config::types::ConfigValue>("", kValue)}};
    auto cfg1_struct =
        std::make_shared<flexi_cfg::config::types::ConfigStruct>(key, 0 /* depth doesn't matter */);
    cfg1_struct->data = std::move(cfg1_inner);
    flexi_cfg::config::types::CfgMap cfg1 = {{cfg1_struct->name, std::move(cfg1_struct)}};

    // cfg2 is:
    //  struct key
    //    key3 = ""
    //    key4 = ""
    flexi_cfg::config::types::CfgMap cfg2_inner = {
        {inner_keys[2], std::make_shared<flexi_cfg::config::types::ConfigValue>("", kValue)},
        {inner_keys[3], std::make_shared<flexi_cfg::config::types::ConfigValue>("", kValue)}};
    auto cfg2_struct =
        std::make_shared<flexi_cfg::config::types::ConfigStruct>(key, 0 /* depth doesn't matter */);
    cfg2_struct->data = std::move(cfg2_inner);
    flexi_cfg::config::types::CfgMap cfg2 = {{cfg2_struct->name, std::move(cfg2_struct)}};

    // The above code should merge into the following:
    //  struct key
    //    key1 = ""
    //    key2 = ""
    //    key3 = ""
    //    key4 = ""
    flexi_cfg::config::types::CfgMap cfg_out{};
    ASSERT_NO_THROW(cfg_out = flexi_cfg::config::helpers::mergeNestedMaps(cfg1, cfg2));

    // The resulting map contains the top level key.
    ASSERT_TRUE(cfg_out.contains(key));
    ASSERT_EQ(cfg_out.size(), 1);

    // The inner struct should contain all of the above keys.
    const auto inner_map =
        dynamic_pointer_cast<flexi_cfg::config::types::ConfigStruct>(cfg_out.at(key));
    for (const auto& k : inner_keys) {
      ASSERT_TRUE(inner_map->data.contains(k));
    }
  }
  {
    flexi_cfg::logger::error("-----------------------------------------------------");
    // This test should fail due to an exception
    const std::string key = "key";
    const std::vector<std::string> inner_keys = {"key1", "key2", "key3"};
    // cfg1 is:
    //  struct key
    //    key1 = ""
    //    key2 = ""
    flexi_cfg::config::types::CfgMap cfg1_inner = {
        {inner_keys[0], std::make_shared<flexi_cfg::config::types::ConfigValue>("", kValue)},
        {inner_keys[1], std::make_shared<flexi_cfg::config::types::ConfigValue>("", kValue)}};
    auto cfg1_struct =
        std::make_shared<flexi_cfg::config::types::ConfigStruct>(key, 0 /* depth doesn't matter */);
    cfg1_struct->data = std::move(cfg1_inner);
    flexi_cfg::config::types::CfgMap cfg1 = {{cfg1_struct->name, std::move(cfg1_struct)}};

    // cfg2 is:
    //  struct key
    //    key3 = ""
    //    key2 = ""  <-- Duplicate key here, will fail.
    flexi_cfg::config::types::CfgMap cfg2_inner = {
        {inner_keys[2], std::make_shared<flexi_cfg::config::types::ConfigValue>("", kValue)},
        // This key is duplicated in the struct above (cfg1_inner), which will cause a failure.
        {inner_keys[1], std::make_shared<flexi_cfg::config::types::ConfigValue>("", kValue)}};
    auto cfg2_struct =
        std::make_shared<flexi_cfg::config::types::ConfigStruct>(key, 0 /* depth doesn't matter */);
    cfg2_struct->data = std::move(cfg2_inner);
    flexi_cfg::config::types::CfgMap cfg2 = {{cfg2_struct->name, std::move(cfg2_struct)}};

    flexi_cfg::config::types::CfgMap cfg_out{};
    EXPECT_THROW(cfg_out = flexi_cfg::config::helpers::mergeNestedMaps(cfg1, cfg2),
                 flexi_cfg::config::DuplicateKeyException);
    flexi_cfg::logger::error("-----------------------------------------------------");
  }
  {
    // Test a multi-nested map
    const std::string key_lvl0 = "outer";
    const std::string key_lvl1 = "inner";
    const std::vector<std::string> keys = {"key1", "key2", "key3", "key4"};
    // NOTE: The value of the keys below don't matter for this test, so just use the empty string

    // cfg1 is:
    //  struct outer
    //    key1 = ""
    //    key2 = ""
    //
    //    struct inner
    //      key1 = ""
    //      key2 = ""
    flexi_cfg::config::types::CfgMap cfg1_lvl2 = {
        {keys[0], std::make_shared<flexi_cfg::config::types::ConfigValue>("", kValue)},
        {keys[1], std::make_shared<flexi_cfg::config::types::ConfigValue>("", kValue)}};
    auto cfg1_inner = std::make_shared<flexi_cfg::config::types::ConfigStruct>(
        key_lvl1, 1 /* depth doesn't matter */);
    cfg1_inner->data = std::move(cfg1_lvl2);
    flexi_cfg::config::types::CfgMap cfg1_lvl1 = {
        {cfg1_inner->name, std::move(cfg1_inner)},
        {keys[0], std::make_shared<flexi_cfg::config::types::ConfigValue>("", kValue)},
        {keys[1], std::make_shared<flexi_cfg::config::types::ConfigValue>("", kValue)}};
    auto cfg1_outer = std::make_shared<flexi_cfg::config::types::ConfigStruct>(
        key_lvl0, 0 /* depth doesn't matter */);
    cfg1_outer->data = std::move(cfg1_lvl1);
    flexi_cfg::config::types::CfgMap cfg1 = {{cfg1_outer->name, std::move(cfg1_outer)}};

    // cfg2 is:
    //  struct outer
    //    key3 = ""
    //    key4 = ""
    //
    //    struct inner
    //      key3 = ""
    //      key4 = ""
    flexi_cfg::config::types::CfgMap cfg2_lvl2 = {
        {keys[2], std::make_shared<flexi_cfg::config::types::ConfigValue>("", kValue)},
        {keys[3], std::make_shared<flexi_cfg::config::types::ConfigValue>("", kValue)}};
    auto cfg2_inner = std::make_shared<flexi_cfg::config::types::ConfigStruct>(
        key_lvl1, 1 /* depth doesn't matter */);
    cfg2_inner->data = std::move(cfg2_lvl2);
    flexi_cfg::config::types::CfgMap cfg2_lvl1 = {
        {cfg2_inner->name, std::move(cfg2_inner)},
        {keys[2], std::make_shared<flexi_cfg::config::types::ConfigValue>("", kValue)},
        {keys[3], std::make_shared<flexi_cfg::config::types::ConfigValue>("", kValue)}};
    auto cfg2_outer = std::make_shared<flexi_cfg::config::types::ConfigStruct>(
        key_lvl0, 0 /* depth doesn't matter */);
    cfg2_outer->data = std::move(cfg2_lvl1);
    flexi_cfg::config::types::CfgMap cfg2 = {{cfg2_outer->name, std::move(cfg2_outer)}};

    // The above code should merge into the following:
    //  struct outer
    //    key1 = ""
    //    key2 = ""
    //    key3 = ""
    //    key4 = ""
    //
    //    struct inner
    //      key1 = ""
    //      key2 = ""
    //      key3 = ""
    //      key4 = ""
    flexi_cfg::config::types::CfgMap cfg_out{};
    ASSERT_NO_THROW(cfg_out = flexi_cfg::config::helpers::mergeNestedMaps(cfg1, cfg2));

    // The inner struct should contain all of the above keys and no more.
    ASSERT_TRUE(cfg_out.contains(key_lvl0));
    ASSERT_EQ(cfg_out.size(), 1);

    const auto outer_map =
        dynamic_pointer_cast<flexi_cfg::config::types::ConfigStruct>(cfg_out.at(key_lvl0));
    for (const auto& k : keys) {
      ASSERT_TRUE(outer_map->data.contains(k));
    }
    ASSERT_TRUE(outer_map->data.contains(key_lvl1));
    const auto inner_map =
        dynamic_pointer_cast<flexi_cfg::config::types::ConfigStruct>(outer_map->data.at(key_lvl1));
    for (const auto& k : keys) {
      ASSERT_TRUE(inner_map->data.contains(k));
    }
  }
}

TEST(ConfigHelpers, structFromReference) {
  {
    // This test should succeed (no exceptions thrown)
    const std::string ref_name = "hx";
    const std::string proto_name = "key";
    const std::vector<std::string> keys = {"key1", "key2", "key3", "key4", "key5"};
    // NOTE: The value of the keys below don't matter for this test, so we'll use some dummy values.

    // Set up reference as such:
    //  reference key as hx
    //    +key1 = 0.14
    //    +key2 = "fizz_buzz"
    //    $KEY3 = "foo"
    //    $KEY4 = "bar"
    auto reference = std::make_shared<flexi_cfg::config::types::ConfigReference>(
        ref_name, proto_name, 4 /* depth doesn't matter */);
    constexpr double MAGIC_NUMBER{0.14};
    reference->data = {
        {keys[0],
         std::make_shared<flexi_cfg::config::types::ConfigValue>(
             std::to_string(MAGIC_NUMBER), flexi_cfg::config::types::Type::kNumber, MAGIC_NUMBER)},
        {keys[1], std::make_shared<flexi_cfg::config::types::ConfigValue>("fizz_buzz", kValue)}};
    reference->ref_vars = {
        {"$KEY3", std::make_shared<flexi_cfg::config::types::ConfigValue>("foo", kValue)},
        {"$KEY4", std::make_shared<flexi_cfg::config::types::ConfigValue>("bar", kValue)}};

    // Set up proto as such:
    //  proto key
    //    key3 = $KEY3
    //    key4 = $KEY4
    std::map<std::string, std::string> expected_proto_values = {
        {keys[2], "$KEY3"}, {keys[3], "$KEY4"}, {keys[4], "-2"}};
    auto proto = std::make_shared<flexi_cfg::config::types::ConfigProto>(
        proto_name, 0 /* depth doesn't matter */);
    proto->data = {
        {keys[2],
         std::make_shared<flexi_cfg::config::types::ConfigVar>(expected_proto_values[keys[2]])},
        {keys[3],
         std::make_shared<flexi_cfg::config::types::ConfigVar>(expected_proto_values[keys[3]])},
        {keys[4], std::make_shared<flexi_cfg::config::types::ConfigValue>(
                      expected_proto_values[keys[4]], kValue)}};

    std::shared_ptr<flexi_cfg::config::types::ConfigStruct> struct_out{};
    ASSERT_NO_THROW(struct_out = flexi_cfg::config::helpers::structFromReference(reference, proto));

    // Ensure that the struct was created properly from the reference
    ASSERT_EQ(reference->name, struct_out->name);
    ASSERT_EQ(reference->depth, struct_out->depth);

    // Reference data should be empty (moved to struct)
    ASSERT_TRUE(reference->data.empty());

    // Ensure that the proto still has all of it's keys.
    for (size_t i = 2; i < keys.size(); ++i) {
      ASSERT_TRUE(proto->data.contains(keys[i]));
    }

    // Verify that the produced struct contains all of the expected keys.
    for (const auto& k : keys) {
      ASSERT_TRUE(struct_out->data.contains(k));
    }

    // We want to ensure that if we modify a value in the struct that came from the proto, the proto
    // value isn't modified.
    for (auto& kv : struct_out->data) {
      if (kv.second->type == flexi_cfg::config::types::Type::kVar) {
        auto var = dynamic_pointer_cast<flexi_cfg::config::types::ConfigVar>(kv.second);
        if (reference->ref_vars.contains(var->name)) {
          kv.second = reference->ref_vars[var->name];
        }
      }
    }

    // Check that :the contents are as expected.
    for (const auto& kv : proto->data) {
      if (kv.second->type == flexi_cfg::config::types::Type::kVar) {
        EXPECT_EQ(expected_proto_values[kv.first],
                  dynamic_pointer_cast<flexi_cfg::config::types::ConfigVar>(kv.second)->name);
      } else if (kv.second->type == flexi_cfg::config::types::Type::kValue) {
        EXPECT_EQ(expected_proto_values[kv.first],
                  dynamic_pointer_cast<flexi_cfg::config::types::ConfigValue>(kv.second)->value);
      } else {
        EXPECT_TRUE(false) << "Unexpected type "
                           << magic_enum::enum_name<flexi_cfg::config::types::Type>(kv.second->type)
                           << " found in proto!";
      }
    }
  }
  {
    // TODO(miker2): Consider adding a more complex test with a proto that has nested values.
  }
}

TEST(ConfigHelpers, replaceVarInStr) {
  {
    const std::string input = "this.is.a.$VAR";
    const std::string expected = "this.is.a.var";

    const flexi_cfg::config::types::RefMap ref_vars = {
        {"$VAR", std::make_shared<flexi_cfg::config::types::ConfigValue>(R"("var")", kValue)}};

    const auto output = flexi_cfg::config::helpers::replaceVarInStr(input, ref_vars);

    EXPECT_EQ(output, expected);
  }

  {
    // Now with a braced var
    const std::string input = "this.is.a.${VAR}";
    const std::string expected = "this.is.a.var";

    const flexi_cfg::config::types::RefMap ref_vars = {
        {"$VAR", std::make_shared<flexi_cfg::config::types::ConfigValue>(R"("var")", kValue)}};

    const auto output = flexi_cfg::config::helpers::replaceVarInStr(input, ref_vars);

    EXPECT_EQ(output, expected);
  }

  {
    // Two vars, one brace, one not.
    const std::string input = "this $CONTAINS_two_${VARS}";
    const std::string expected = "this contains_two_vars";

    const flexi_cfg::config::types::RefMap ref_vars = {
        {"$VARS", std::make_shared<flexi_cfg::config::types::ConfigValue>(R"("vars")", kValue)},
        {"$EXTRA", std::make_shared<flexi_cfg::config::types::ConfigValue>(
                       "extra unused", kValue)},  // extra, unused
        {"$CONTAINS",
         std::make_shared<flexi_cfg::config::types::ConfigValue>(R"("contains")", kValue)}};

    const auto output = flexi_cfg::config::helpers::replaceVarInStr(input, ref_vars);

    EXPECT_EQ(output, expected);
  }

  {
    // The input string may be a value lookup string:
    const std::string input = "$($LOTS.$OF.${VARS})";
    const std::string expected = "$(a.value.lookup)";

    const flexi_cfg::config::types::RefMap ref_vars = {
        {"$VARS", std::make_shared<flexi_cfg::config::types::ConfigValue>(R"("lookup")", kValue)},
        {"$LOTS", std::make_shared<flexi_cfg::config::types::ConfigValue>(R"("a")", kValue)},
        {"$OF", std::make_shared<flexi_cfg::config::types::ConfigValue>(R"("value")", kValue)},
        // A a few extra vars here. They won't be used, and shouldn't affect the output.
        {"$EXTRA", std::make_shared<flexi_cfg::config::types::ConfigValue>("Extra", kValue)},
        {"$KEYS", std::make_shared<flexi_cfg::config::types::ConfigValue>(" keys ", kValue)}};

    const auto output = flexi_cfg::config::helpers::replaceVarInStr(input, ref_vars);

    EXPECT_EQ(output, expected);
  }

  {
    const std::string input = "this.$SHOULD_PASS.the.test";  // <-- This is ambiguous
    const std::string expected = "this.should_PASS.the.test";

    const flexi_cfg::config::types::RefMap ref_vars = {
        {"$SHOULD",
         std::make_shared<flexi_cfg::config::types::ConfigValue>(R"("should")", kValue)}};

    const auto output = flexi_cfg::config::helpers::replaceVarInStr(input, ref_vars);

    EXPECT_EQ(output, expected);
  }

  {
    const std::string input = "this.${SHOULD_NOT}.match";  // <-- less ambiguous
    const std::string expected = "this.should_NOT.match";
    ;

    const flexi_cfg::config::types::RefMap ref_vars = {
        {"$SHOULD",
         std::make_shared<flexi_cfg::config::types::ConfigValue>(R"("should")", kValue)}};

    const auto output = flexi_cfg::config::helpers::replaceVarInStr(input, ref_vars);

    EXPECT_NE(output, expected);
  }
}

// TODO(miker2): Test for replaceProtoVar

auto generateConfig() -> flexi_cfg::config::types::CfgMap {
  /* Build up an example config structure

     ref = $(outer.inner.key1)
     top_level = 10
     struct outer {
       struct inner {
         key1 = "key1"
         key2 = $(ref)
       }
       a_key = -9.87
     }
   */
  auto inner = std::make_shared<flexi_cfg::config::types::ConfigStruct>("inner", 0);
  inner->data = {{"key1", std::make_shared<flexi_cfg::config::types::ConfigValue>("10", kValue)},
                 {"key2", std::make_shared<flexi_cfg::config::types::ConfigValueLookup>("ref")}};
  auto outer = std::make_shared<flexi_cfg::config::types::ConfigStruct>("outer", 0);
  outer->data = {
      {inner->name, inner},
      {"a_key", std::make_shared<flexi_cfg::config::types::ConfigValue>("-9.87", kValue)}};

  flexi_cfg::config::types::CfgMap cfg = {
      {"ref", std::make_shared<flexi_cfg::config::types::ConfigValueLookup>("outer.inner.key1")},
      {"top_level", std::make_shared<flexi_cfg::config::types::ConfigValue>("10", kValue)},
      {outer->name, outer}};

  return cfg;
}

TEST(ConfigHelpers, getNestedConfig) {
  auto cfg = generateConfig();
  // NOTE: getNestedConfig always returns the "parent" of the last key
  {
    const auto out = flexi_cfg::config::helpers::getNestedConfig(cfg, "outer.inner.key1");
    ASSERT_NE(out, nullptr);
    EXPECT_EQ(out->name, "inner");
  }
  {
    const auto out = flexi_cfg::config::helpers::getNestedConfig(cfg, {"outer", "inner", "key2"});
    ASSERT_NE(out, nullptr);
    EXPECT_EQ(out->name, "inner");
  }
  {
    // "outer.inner.key1" is a key, not a struct, so this will fail.
    EXPECT_THROW(std::ignore = flexi_cfg::config::helpers::getNestedConfig(
                     cfg, "outer.inner.key1.doesnt_exist"),
                 flexi_cfg::config::InvalidTypeException);
  }
  {
    // This is an odd one! Since 'getNestedConfig' only checks for the parent, it doesn't matter if
    // the last key doesn't exist, which in this case, 'does_not_exist' doesn't exist.
    const auto out = flexi_cfg::config::helpers::getNestedConfig(cfg, "outer.inner.does_not_exist");
    ASSERT_NE(out, nullptr);
    EXPECT_EQ(out->name, "inner");
  }
  {
    // This still works (the first argument doesn't need to be a top level entry)
    const auto out = flexi_cfg::config::helpers::getNestedConfig(
        dynamic_pointer_cast<flexi_cfg::config::types::ConfigStructLike>(cfg["outer"])->data,
        "inner.key1");
    ASSERT_NE(out, nullptr);
    EXPECT_EQ(out->name, "inner");
  }
  {
    const auto out = flexi_cfg::config::helpers::getNestedConfig(cfg, "outer.a_key");
    ASSERT_NE(out, nullptr);
    EXPECT_EQ(out->name, "outer");
  }
  {
    // "does_not_exist" is not a valid key within "outer". This would work if 'foo' wasn't at the
    // end of this.
    EXPECT_THROW(
        std::ignore = flexi_cfg::config::helpers::getNestedConfig(cfg, "outer.does_not_exist.foo"),
        flexi_cfg::config::InvalidKeyException);
  }
  {
    // It really doesn't matter what we pass in as the second argument as long as it results in a
    // single key when split!
    ASSERT_EQ(flexi_cfg::config::helpers::getNestedConfig(cfg, "top_level"), nullptr);
    ASSERT_EQ(flexi_cfg::config::helpers::getNestedConfig(cfg, "outer"), nullptr);
    ASSERT_EQ(flexi_cfg::config::helpers::getNestedConfig(cfg, ""), nullptr);
    std::vector<std::string> keys = {"outer"};
    ASSERT_EQ(flexi_cfg::config::helpers::getNestedConfig(cfg, keys), nullptr);
    keys = {"top_level"};
    ASSERT_EQ(flexi_cfg::config::helpers::getNestedConfig(cfg, keys), nullptr);
  }
}

TEST(ConfigHelpers, getConfigValue) {
  auto cfg = generateConfig();

  {
    const auto val = flexi_cfg::config::helpers::getConfigValue(cfg, {"top_level"});
    EXPECT_EQ(val->type, flexi_cfg::config::types::Type::kValue);
  }
  {
    // It doesn't matter what type the value is, we just want whatever is at the key below
    auto val = flexi_cfg::config::helpers::getConfigValue(cfg, {"ref"});
    ASSERT_EQ(val->type, flexi_cfg::config::types::Type::kValueLookup);
    auto val_lookup = dynamic_pointer_cast<flexi_cfg::config::types::ConfigValueLookup>(val);
    ASSERT_NE(val_lookup, nullptr);
    EXPECT_NO_THROW(flexi_cfg::config::helpers::getConfigValue(cfg, val_lookup));
  }
  {
    const auto val = flexi_cfg::config::helpers::getConfigValue(cfg, {"outer"});
    EXPECT_EQ(val->type, flexi_cfg::config::types::Type::kStruct);
  }
  {
    const auto val = flexi_cfg::config::helpers::getConfigValue(cfg, {"outer", "inner"});
    EXPECT_EQ(val->type, flexi_cfg::config::types::Type::kStruct);
  }
  {
    EXPECT_NO_THROW(
        std::ignore = flexi_cfg::config::helpers::getConfigValue(cfg, {"outer", "inner", "key1"}));
    EXPECT_NO_THROW(
        std::ignore = flexi_cfg::config::helpers::getConfigValue(cfg, {"outer", "inner", "key2"}));
    EXPECT_THROW(std::ignore = flexi_cfg::config::helpers::getConfigValue(
                     cfg, {"outer", "inner", "doesnt_exist"}),
                 flexi_cfg::config::InvalidKeyException);
    EXPECT_THROW(
        std::ignore = flexi_cfg::config::helpers::getConfigValue(cfg, {"outer", "doesnt_exist"}),
        flexi_cfg::config::InvalidKeyException);
  }
}

TEST(ConfigHelpers, resolveVarRefs) {
  {
    auto cfg = generateConfig();
    auto key1 = flexi_cfg::config::helpers::getConfigValue(cfg, {"outer", "inner", "key1"});
    ASSERT_EQ(key1->type, flexi_cfg::config::types::Type::kValue);
    const std::string expected_value =
        dynamic_pointer_cast<flexi_cfg::config::types::ConfigValue>(key1)->value;

    EXPECT_NO_THROW(flexi_cfg::config::helpers::resolveVarRefs(cfg, cfg));

    ASSERT_EQ(cfg["ref"]->type, flexi_cfg::config::types::Type::kValue);
    EXPECT_EQ(dynamic_pointer_cast<flexi_cfg::config::types::ConfigValue>(cfg["ref"])->value,
              expected_value);

    auto key2 = flexi_cfg::config::helpers::getConfigValue(cfg, {"outer", "inner", "key2"});
    ASSERT_EQ(key2->type, flexi_cfg::config::types::Type::kValue);
    ASSERT_NE(dynamic_pointer_cast<flexi_cfg::config::types::ConfigValue>(key2), nullptr);
    EXPECT_EQ(dynamic_pointer_cast<flexi_cfg::config::types::ConfigValue>(key2)->value,
              expected_value);
  }
  {
    flexi_cfg::config::types::CfgMap cfg = {
        {"foo", std::make_shared<flexi_cfg::config::types::ConfigValueLookup>("bar")},
        {"bar", std::make_shared<flexi_cfg::config::types::ConfigValueLookup>("baz")},
        {"baz", std::make_shared<flexi_cfg::config::types::ConfigValueLookup>("foo")}};

    EXPECT_THROW(flexi_cfg::config::helpers::resolveVarRefs(cfg, cfg),
                 flexi_cfg::config::CyclicReferenceException);
  }
}

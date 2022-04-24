#include "config_helpers.h"

#include <gtest/gtest.h>

#include <magic_enum.hpp>

#include "config_classes.h"

namespace {
template <typename T, typename... Args>
void testIsStructLike(Args&&... args) {
  const auto obj = std::make_shared<T>(args...);
  EXPECT_EQ(config::helpers::isStructLike(obj),
            dynamic_pointer_cast<config::types::ConfigStructLike>(obj) != nullptr);
}
}  // namespace

TEST(config_helpers_test, isStructLike) {
  testIsStructLike<config::types::ConfigValue>("");

  testIsStructLike<config::types::ConfigValueLookup>("");

  testIsStructLike<config::types::ConfigVar>("");

  testIsStructLike<config::types::ConfigStruct>("struct", 0);

  testIsStructLike<config::types::ConfigProto>("proto", 0);

  testIsStructLike<config::types::ConfigReference>("reference", "proto", 0);
}

TEST(config_helpers_test, checkForErrors) {
  {
    // This test should fail due to duplicate keys
    const std::string key = "key1";
    config::types::CfgMap cfg1 = {
        {key, std::make_shared<config::types::ConfigValue>("13", config::types::Type::kNumber)}};
    config::types::CfgMap cfg2 = {{key, std::make_shared<config::types::ConfigValue>(
                                            "string", config::types::Type::kString)}};

    EXPECT_THROW(config::helpers::checkForErrors(cfg1, cfg2, key), config::DuplicateKeyException);
  }
  {
    // This test should fail due to mismatched types (1 struct-like, the other not)
    const std::string key = "key1";
    config::types::CfgMap cfg1 = {
        {key, std::make_shared<config::types::ConfigValue>("13", config::types::Type::kNumber)}};
    config::types::CfgMap cfg2 = {
        {key, std::make_shared<config::types::ConfigStruct>(key, 0 /* depth doesn't matter */)}};

    EXPECT_THROW(config::helpers::checkForErrors(cfg1, cfg2, key), config::MismatchKeyException);
  }
  {
    // This test should fail due to mismatched types (both struct-like, but different types)
    const std::string key = "key1";
    config::types::CfgMap cfg1 = {
        {key, std::make_shared<config::types::ConfigProto>(key, 0 /* depth doesn't matter */)}};
    config::types::CfgMap cfg2 = {
        {key, std::make_shared<config::types::ConfigStruct>(key, 0 /* depth doesn't matter */)}};

    EXPECT_THROW(config::helpers::checkForErrors(cfg1, cfg2, key), config::MismatchTypeException);
  }
  {
    // This test should be successful
    const std::string key = "key1";
    config::types::CfgMap cfg1 = {
        {key, std::make_shared<config::types::ConfigStruct>(key, 0 /* depth doesn't matter */)}};
    config::types::CfgMap cfg2 = {
        {key, std::make_shared<config::types::ConfigStruct>(key, 0 /* depth doesn't matter */)}};

    EXPECT_NO_THROW(config::helpers::checkForErrors(cfg1, cfg2, key));
  }
  {
    // This test should fail (no common keys)
    const std::string key1 = "key1";
    config::types::CfgMap cfg1 = {
        {key1, std::make_shared<config::types::ConfigStruct>(key1, 0 /* depth doesn't matter */)}};

    const std::string key2 = "key2";
    config::types::CfgMap cfg2 = {
        {key2, std::make_shared<config::types::ConfigStruct>(key2, 0 /* depth doesn't matter */)}};

    EXPECT_THROW(config::helpers::checkForErrors(cfg1, cfg2, key1), std::out_of_range)
        << "cfg1: " << cfg1 << ", cfg2: " << cfg2;
    EXPECT_THROW(config::helpers::checkForErrors(cfg1, cfg2, key2), std::out_of_range)
        << "cfg1: " << cfg1 << ", cfg2: " << cfg2;
  }
}

TEST(config_helpers_test, mergeNestedMaps) {
  {
    // This test should succeed (no exceptions thrown)
    const std::string key = "key";
    const std::vector<std::string> inner_keys = {"key1", "key2", "key3", "key4"};
    // NOTE: The value of the keys below don't matter for this test, so just use the empty string

    // cfg1 is:
    //  struct key
    //    key1 = ""
    //    key2 = ""
    config::types::CfgMap cfg1_inner = {
        {inner_keys[0], std::make_shared<config::types::ConfigValue>("")},
        {inner_keys[1], std::make_shared<config::types::ConfigValue>("")}};
    auto cfg1_struct =
        std::make_shared<config::types::ConfigStruct>(key, 0 /* depth doesn't matter */);
    cfg1_struct->data = std::move(cfg1_inner);
    config::types::CfgMap cfg1 = {{cfg1_struct->name, std::move(cfg1_struct)}};

    // cfg2 is:
    //  struct key
    //    key3 = ""
    //    key4 = ""
    config::types::CfgMap cfg2_inner = {
        {inner_keys[2], std::make_shared<config::types::ConfigValue>("")},
        {inner_keys[3], std::make_shared<config::types::ConfigValue>("")}};
    auto cfg2_struct =
        std::make_shared<config::types::ConfigStruct>(key, 0 /* depth doesn't matter */);
    cfg2_struct->data = std::move(cfg2_inner);
    config::types::CfgMap cfg2 = {{cfg2_struct->name, std::move(cfg2_struct)}};

    // The above code should merge into the following:
    //  struct key
    //    key1 = ""
    //    key2 = ""
    //    key3 = ""
    //    key4 = ""
    config::types::CfgMap cfg_out{};
    ASSERT_NO_THROW(cfg_out = config::helpers::mergeNestedMaps(cfg1, cfg2));

    // The resulting map contains the top level key.
    ASSERT_TRUE(cfg_out.contains(key));
    ASSERT_EQ(cfg_out.size(), 1);

    // The inner struct should contain all of the above keys.
    const auto inner_map = dynamic_pointer_cast<config::types::ConfigStruct>(cfg_out.at(key));
    for (const auto& k : inner_keys) {
      ASSERT_TRUE(inner_map->data.contains(k));
    }
  }
  {
    // This test should fail due to an exception
    const std::string key = "key";
    const std::vector<std::string> inner_keys = {"key1", "key2", "key3"};
    // cfg1 is:
    //  struct key
    //    key1 = ""
    //    key2 = ""
    config::types::CfgMap cfg1_inner = {
        {inner_keys[0], std::make_shared<config::types::ConfigValue>("")},
        {inner_keys[1], std::make_shared<config::types::ConfigValue>("")}};
    auto cfg1_struct =
        std::make_shared<config::types::ConfigStruct>(key, 0 /* depth doesn't matter */);
    cfg1_struct->data = std::move(cfg1_inner);
    config::types::CfgMap cfg1 = {{cfg1_struct->name, std::move(cfg1_struct)}};

    // cfg1 is:
    //  struct key
    //    key3 = ""
    //    key2 = ""  <-- Duplicate key here, will fail.
    config::types::CfgMap cfg2_inner = {
        {inner_keys[2], std::make_shared<config::types::ConfigValue>("")},
        // This key is duplicated in the struct above (cfg1_inner), which will cause a failure.
        {inner_keys[1], std::make_shared<config::types::ConfigValue>("")}};
    auto cfg2_struct =
        std::make_shared<config::types::ConfigStruct>(key, 0 /* depth doesn't matter */);
    cfg2_struct->data = std::move(cfg2_inner);
    config::types::CfgMap cfg2 = {{cfg2_struct->name, std::move(cfg2_struct)}};

    config::types::CfgMap cfg_out{};
    ASSERT_THROW(cfg_out = config::helpers::mergeNestedMaps(cfg1, cfg2),
                 config::DuplicateKeyException);
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
    config::types::CfgMap cfg1_lvl2 = {{keys[0], std::make_shared<config::types::ConfigValue>("")},
                                       {keys[1], std::make_shared<config::types::ConfigValue>("")}};
    auto cfg1_inner =
        std::make_shared<config::types::ConfigStruct>(key_lvl1, 1 /* depth doesn't matter */);
    cfg1_inner->data = std::move(cfg1_lvl2);
    config::types::CfgMap cfg1_lvl1 = {{cfg1_inner->name, std::move(cfg1_inner)},
                                       {keys[0], std::make_shared<config::types::ConfigValue>("")},
                                       {keys[1], std::make_shared<config::types::ConfigValue>("")}};
    auto cfg1_outer =
        std::make_shared<config::types::ConfigStruct>(key_lvl0, 0 /* depth doesn't matter */);
    cfg1_outer->data = std::move(cfg1_lvl1);
    config::types::CfgMap cfg1 = {{cfg1_outer->name, std::move(cfg1_outer)}};

    // cfg2 is:
    //  struct outer
    //    key3 = ""
    //    key4 = ""
    //
    //    struct inner
    //      key3 = ""
    //      key4 = ""
    config::types::CfgMap cfg2_lvl2 = {{keys[2], std::make_shared<config::types::ConfigValue>("")},
                                       {keys[3], std::make_shared<config::types::ConfigValue>("")}};
    auto cfg2_inner =
        std::make_shared<config::types::ConfigStruct>(key_lvl1, 1 /* depth doesn't matter */);
    cfg2_inner->data = std::move(cfg2_lvl2);
    config::types::CfgMap cfg2_lvl1 = {{cfg2_inner->name, std::move(cfg2_inner)},
                                       {keys[2], std::make_shared<config::types::ConfigValue>("")},
                                       {keys[3], std::make_shared<config::types::ConfigValue>("")}};
    auto cfg2_outer =
        std::make_shared<config::types::ConfigStruct>(key_lvl0, 0 /* depth doesn't matter */);
    cfg2_outer->data = std::move(cfg2_lvl1);
    config::types::CfgMap cfg2 = {{cfg2_outer->name, std::move(cfg2_outer)}};

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
    config::types::CfgMap cfg_out{};
    ASSERT_NO_THROW(cfg_out = config::helpers::mergeNestedMaps(cfg1, cfg2));

    // The inner struct should contain all of the above keys and no more.
    ASSERT_TRUE(cfg_out.contains(key_lvl0));
    ASSERT_EQ(cfg_out.size(), 1);

    const auto outer_map = dynamic_pointer_cast<config::types::ConfigStruct>(cfg_out.at(key_lvl0));
    for (const auto& k : keys) {
      ASSERT_TRUE(outer_map->data.contains(k));
    }
    ASSERT_TRUE(outer_map->data.contains(key_lvl1));
    const auto inner_map =
        dynamic_pointer_cast<config::types::ConfigStruct>(outer_map->data.at(key_lvl1));
    for (const auto& k : keys) {
      ASSERT_TRUE(inner_map->data.contains(k));
    }
  }
}

TEST(config_helpers_test, structFromReference) {
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
    auto reference = std::make_shared<config::types::ConfigReference>(ref_name, proto_name,
                                                                      4 /* depth doesn't matter */);
    reference->data = {{keys[0], std::make_shared<config::types::ConfigValue>(
                                     "0.14", config::types::Type::kNumber, 0.14)},
                       {keys[1], std::make_shared<config::types::ConfigValue>("fizz_buzz")}};
    reference->ref_vars = {{"$KEY3", std::make_shared<config::types::ConfigValue>("foo")},
                           {"$KEY4", std::make_shared<config::types::ConfigValue>("bar")}};

    // Set up proto as such:
    //  proto key
    //    key3 = $KEY3
    //    key4 = $KEY4
    std::map<std::string, std::string> expected_proto_values = {
        {keys[2], "$KEY3"}, {keys[3], "$KEY4"}, {keys[4], "-2"}};
    auto proto =
        std::make_shared<config::types::ConfigProto>(proto_name, 0 /* depth doesn't matter */);
    proto->data = {
        {keys[2], std::make_shared<config::types::ConfigVar>(expected_proto_values[keys[2]])},
        {keys[3], std::make_shared<config::types::ConfigVar>(expected_proto_values[keys[3]])},
        {keys[4], std::make_shared<config::types::ConfigValue>(expected_proto_values[keys[4]])}};

    std::shared_ptr<config::types::ConfigStruct> struct_out{};
    ASSERT_NO_THROW(struct_out = config::helpers::structFromReference(reference, proto));

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
      if (kv.second->type == config::types::Type::kVar) {
        auto var = dynamic_pointer_cast<config::types::ConfigVar>(kv.second);
        if (reference->ref_vars.contains(var->name)) {
          kv.second = reference->ref_vars[var->name];
        }
      }
    }

    // Check that the contents are as expected.
    for (const auto& kv : proto->data) {
      if (kv.second->type == config::types::Type::kVar) {
        EXPECT_EQ(expected_proto_values[kv.first],
                  dynamic_pointer_cast<config::types::ConfigVar>(kv.second)->name);
      } else if (kv.second->type == config::types::Type::kValue) {
        EXPECT_EQ(expected_proto_values[kv.first],
                  dynamic_pointer_cast<config::types::ConfigValue>(kv.second)->value);
      } else {
        EXPECT_TRUE(false) << "Unexpected type "
                           << magic_enum::enum_name<config::types::Type>(kv.second->type)
                           << " found in proto!";
      }
    }
  }
  {
    // TODO: Consider adding a more complex test with a proto that has nested values.
  }
}

TEST(config_helpers_test, replaceVarInStr) {
  {
    const std::string input = "this.is.a.$VAR";
    const std::string expected = "this.is.a.var";

    const config::types::RefMap ref_vars = {
        {"$VAR", std::make_shared<config::types::ConfigValue>(R"("var")")}};

    const auto output = config::helpers::replaceVarInStr(input, ref_vars);

    EXPECT_EQ(output, expected);
  }

  {
    // Now with a braced var
    const std::string input = "this.is.a.${VAR}";
    const std::string expected = "this.is.a.var";

    const config::types::RefMap ref_vars = {
        {"$VAR", std::make_shared<config::types::ConfigValue>(R"("var")")}};

    const auto output = config::helpers::replaceVarInStr(input, ref_vars);

    EXPECT_EQ(output, expected);
  }

  {
    // Two vars, one brace, one not.
    const std::string input = "this $CONTAINS_two_${VARS}";
    const std::string expected = "this contains_two_vars";

    const config::types::RefMap ref_vars = {
        {"$VARS", std::make_shared<config::types::ConfigValue>(R"("vars")")},
        {"$EXTRA", std::make_shared<config::types::ConfigValue>("extra unused")},  // extra, unused
        {"$CONTAINS", std::make_shared<config::types::ConfigValue>(R"("contains")")}};

    const auto output = config::helpers::replaceVarInStr(input, ref_vars);

    EXPECT_EQ(output, expected);
  }

  {
    // The input string may be a value lookup string:
    const std::string input = "$($LOTS.$OF.${VARS})";
    const std::string expected = "$(a.value.lookup)";

    const config::types::RefMap ref_vars = {
        {"$VARS", std::make_shared<config::types::ConfigValue>(R"("lookup")")},
        {"$LOTS", std::make_shared<config::types::ConfigValue>(R"("a")")},
        {"$OF", std::make_shared<config::types::ConfigValue>(R"("value")")},
        // A a few extra vars here. They won't be used, and shouldn't affect the output.
        {"$EXTRA", std::make_shared<config::types::ConfigValue>("Extra")},
        {"$KEYS", std::make_shared<config::types::ConfigValue>(" keys ")}};

    const auto output = config::helpers::replaceVarInStr(input, ref_vars);

    EXPECT_EQ(output, expected);
  }

  {
    const std::string input = "this.$SHOULD_PASS.the.test";  // <-- This is ambiguous
    const std::string expected = "this.should_PASS.the.test";

    const config::types::RefMap ref_vars = {
        {"$SHOULD", std::make_shared<config::types::ConfigValue>(R"("should")")}};

    const auto output = config::helpers::replaceVarInStr(input, ref_vars);

    EXPECT_EQ(output, expected);
  }

  {
    const std::string input = "this.${SHOULD_NOT}.match";  // <-- less ambiguous
    const std::string expected = "this.should_NOT.match";
    ;

    const config::types::RefMap ref_vars = {
        {"$SHOULD", std::make_shared<config::types::ConfigValue>(R"("should")")}};

    const auto output = config::helpers::replaceVarInStr(input, ref_vars);

    EXPECT_NE(output, expected);
  }
}

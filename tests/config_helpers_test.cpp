#include "config_helpers.h"

#include <gtest/gtest.h>

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

    // cfg1 bcomes:
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

    // cfg2 bcomes:
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

    // The inner struct should contain all of the above keys.
    ASSERT_TRUE(cfg_out.contains(key));

    const auto inner_map = dynamic_pointer_cast<config::types::ConfigStruct>(cfg_out.at(key));
    for (const auto& k : inner_keys) {
      ASSERT_TRUE(inner_map->data.contains(k));
    }
  }
  {
    // This test should fail due to an exception
    const std::string key = "key";
    const std::vector<std::string> inner_keys = {"key1", "key2", "key3"};
    // cfg1 bcomes:
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

    // cfg1 bcomes:
    //  struct key
    //    key3 = ""
    //    key2 = ""  <-- Duplicate key here
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

    // cfg1 bcomes:
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

    // cfg2 bcomes:
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

    // The inner struct should contain all of the above keys.
    ASSERT_TRUE(cfg_out.contains(key_lvl0));

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

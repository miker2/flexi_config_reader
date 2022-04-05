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

TEST(config_helpers_test, mergeNestedMaps) {}

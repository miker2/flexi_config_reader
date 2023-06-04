#include "flexi_cfg/details/ordered_map.h"

#include <fmt/format.h>
#include <gtest/gtest.h>

#include <string>

#include "flexi_cfg/utils.h"

namespace {
using MyMap = std::unordered_map<std::string, int>;
using OMap = flexi_cfg::details::ordered_map<std::string, int>;
}  // namespace

namespace fmt {
template <>
struct formatter<OMap::value_type> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const OMap::value_type& kv, FormatContext& ctx) const {
    return format_to(ctx.out(), "{} = {}", kv.first, kv.second);
  }
};
}  // namespace fmt

TEST(OrderedMap, Test) {
  OMap map = {{"this", 0}, {"is", 1},  {"a", 2},      {"test", 3}, {"to", 4},
              {"see", 5},  {"how", 6}, {"things", 7}, {"work", 8}};

  fmt::print("value_type: {}\n", flexi_cfg::utils::getTypeName<MyMap::value_type>());

  fmt::print("value_type: {}\n", flexi_cfg::utils::getTypeName<OMap::value_type>());

  fmt::print("keys: {}\n", fmt::join(map.keys(), ", "));
  fmt::print("map: \n  {}\n", fmt::join(map.map(), "\n  "));

  fmt::print("----- const iterator check: -----\n");
  for (const auto& [key, value] : map) {
    fmt::print("key: {}, value: {}\n", key, value);
  }

  fmt::print("----- non-const iterator check: -----\n");
  for (auto& [key, value] : map) {
    fmt::print("key: {}, value: {}\n", key, value);
  }

  {  // Insertion test. Need to test the other signatures of this call.
    auto ret = map.insert(OMap::value_type{"new 1", -1});
    fmt::print("success: {}, it: {}\n", ret.second, *ret.first);
    auto fail = map.insert(OMap::value_type{"test", -1});
    fmt::print("Success: {}, it: {}\n", fail.second, *fail.first);
    auto ret2 = map.insert(OMap::value_type{"see", -1});
    fmt::print("success: {}, it: {}\n", ret2.second, *ret2.first);
  }

  {
    MyMap test = {{"a", 0}, {"b", 1}, {"c", 2}};
    auto suc = test.insert({"d", 3});
    fmt::print("success: {}, it: {}\n", suc.second, *suc.first);
    auto fail = test.insert({"b", 4});
    fmt::print("success: {}, ", fail.second);
    if (fail.first != std::end(test)) {
      fmt::print("it: {}", *fail.first);
    }
    fmt::print("\n");
  }
}

TEST(OrderedMap, constructors) {
  using BasicMap = flexi_cfg::details::ordered_map<int, double>;

  BasicMap empty;
  EXPECT_TRUE(empty.empty());
  EXPECT_EQ(empty.size(), 0);

  // Initializer list construction
  BasicMap map0({{1, 1.0}, {2, 2.0}, {3, 3.0}});

  // Initializer list assignment construction
  BasicMap map1 = {{1, 1.0}, {2, 2.0}, {3, 3.0}};
  EXPECT_EQ(map1.size(), 3);

  // Copy constructor
  BasicMap map2(map1);
  EXPECT_EQ(map2.size(), 3);

  // Const copy constructor
  const BasicMap map3_const(map1);
  EXPECT_EQ(map3_const.size(), 3);

  // Iterator constructor
  BasicMap map4(std::begin(map1), std::end(map1));
  EXPECT_EQ(map4.size(), 3);

  // Move constructor
  BasicMap map3(std::move(map2));
  EXPECT_EQ(map3.size(), 3);

  // Construct from a std::vector<std::pair<K, V>>
  std::vector<std::pair<int, double>> vec = {{1, 1.0}, {2, 2.0}, {3, 3.0}};
  BasicMap map5(std::begin(vec), std::end(vec));
  EXPECT_EQ(map5.size(), vec.size());
}

TEST(OrderedMap, order) {
  // Create a map, and ensure that the order is preserved.
  OMap map = {{"this", 0}, {"is", 1},  {"a", 2},      {"test", 3}, {"to", 4},
              {"see", 5},  {"how", 6}, {"things", 7}, {"work", 8}};
  std::vector<std::string> expected_keys = {"this", "is",  "a",      "test", "to",
                                            "see",  "how", "things", "work"};
  for (const auto& [key, idx] : map) {
    EXPECT_EQ(key, expected_keys[idx]);
  }
}

TEST(OrderedMap, iterators) {
  std::vector<std::string> expected_keys = {"this", "is",  "a",      "test", "to",
                                            "see",  "how", "things", "work"};
  auto build_map = [](const std::vector<std::string>& keys) {
    OMap map;
    for (size_t i = 0; i < keys.size(); ++i) {
      map.insert({keys[i], i});
    }
    return map;
  };
  {
    OMap map = build_map(expected_keys);
    for (auto it = map.begin(); it != map.end(); ++it) {
      EXPECT_EQ(it->first, expected_keys[it->second]);
      EXPECT_EQ(std::distance(std::begin(map), it), it->second);
    }
  }
  {
    const OMap map = build_map(expected_keys);
    for (auto it = map.begin(); it != map.end(); ++it) {
      EXPECT_EQ(it->first, expected_keys[it->second]);
      EXPECT_EQ(std::distance(std::begin(map), it), it->second);
    }
  }
}

TEST(OrderedMap, insert) {
  OMap map = {{"this", 0}, {"is", 1},  {"a", 2},      {"test", 3}, {"to", 4},
              {"see", 5},  {"how", 6}, {"things", 7}, {"work", 8}};
  {
    // Insert a new element. Check that it succeeds and that the returned iterator points to the new
    // element.
    const auto& ret = map.insert(OMap::value_type{"new 1", -1});
    EXPECT_TRUE(ret.second);
    const auto& it = ret.first;
    EXPECT_EQ(it->first, "new 1");
    EXPECT_EQ(it->second, -1);
  }
  {
    // Attempt to insert and element with a key that already exists. Check that it fails and that
    // the returned iterator points to the existing element.
    auto fail = map.insert(OMap::value_type{"test", -1});
    EXPECT_FALSE(fail.second);
    EXPECT_EQ(fail.first->first, "test");
    EXPECT_EQ(fail.first->second, 3);
  }
}
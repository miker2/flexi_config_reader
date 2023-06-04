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

  map5.clear();
  EXPECT_TRUE(map5.empty());
  EXPECT_EQ(map5.size(), 0);
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
    // Iterate using non-const iterators.
    for (auto it = map.begin(); it != map.end(); ++it) {
      EXPECT_EQ(it->first, expected_keys[it->second]);
      EXPECT_EQ(std::distance(std::begin(map), it), it->second);
    }
    // Iterate using const iterators.
    for (auto it = map.cbegin(); it != map.cend(); ++it) {
      EXPECT_EQ(it->first, expected_keys[it->second]);
      EXPECT_EQ(std::distance(std::cbegin(map), it), it->second);
    }
  }
  {
    const OMap map = build_map(expected_keys);
    // Iterate using const iterators.
    for (auto it = map.begin(); it != map.end(); ++it) {
      EXPECT_EQ(it->first, expected_keys[it->second]);
      EXPECT_EQ(std::distance(std::begin(map), it), it->second);
    }
    // Iterate using const iterators.
    for (auto it = map.cbegin(); it != map.cend(); ++it) {
      EXPECT_EQ(it->first, expected_keys[it->second]);
      EXPECT_EQ(std::distance(std::cbegin(map), it), it->second);
    }
  }
}

TEST(OrderedMap, insert) {
  OMap map = {{"this", 0}, {"is", 1},  {"a", 2},      {"test", 3}, {"to", 4},
              {"see", 5},  {"how", 6}, {"things", 7}, {"work", 8}};
  // This will test both paths of all the insert overloads.
  {
    // Insert a new element. Check that it succeeds and that the returned iterator points to the new
    // element.
    const OMap::value_type pair{"new 1", -1};
    const auto& ret = map.insert(pair);
    EXPECT_TRUE(ret.second);
    const auto& it = ret.first;
    EXPECT_EQ(it->first, "new 1");
    EXPECT_EQ(it->second, -1);

    // This time, try to insert an element with the same key, different value.
    const OMap::value_type pair2{pair.first, -10};
    const auto& fail = map.insert(pair2);
    EXPECT_FALSE(fail.second);
    EXPECT_EQ(fail.first->first, pair.first);
    EXPECT_EQ(fail.first->second, pair.second);
  }
  {
    // Attempt to insert and element with a key that already exists. Check that it fails and that
    // the returned iterator points to the existing element.
    auto fail = map.insert(OMap::value_type{"test", -1});
    EXPECT_FALSE(fail.second);
    EXPECT_EQ(fail.first->first, "test");
    EXPECT_EQ(fail.first->second, 3);

    // Now, insert a key that doesn't exist.
    auto ret = map.insert(OMap::value_type{"new 2", -1});
    EXPECT_TRUE(ret.second);
    EXPECT_EQ(ret.first->first, "new 2");
    EXPECT_EQ(ret.first->second, -1);
  }
  {
    // Use an int16_t here since it is convertible to an int, and thus will call the templated
    // 'insert' overload.

    // This one will pass
    const std::pair<std::string, int16_t> pair{"foo", 20};
    const auto& ret = map.insert(pair);
    EXPECT_TRUE(ret.second);
    EXPECT_EQ(ret.first->first, pair.first);
    EXPECT_EQ(ret.first->second, pair.second);

    // This one will fail because the key already exists
    const std::pair<std::string, int16_t> pair2{pair.first, 30};
    const auto& fail = map.insert(pair2);
    EXPECT_FALSE(fail.second);
    EXPECT_EQ(fail.first->first, pair.first);
    EXPECT_EQ(fail.first->second, pair.second);
  }
}

TEST(OrderedMap, insert_or_assign) {
  OMap map({{"one", 1}, {"two", 2}, {"three", 3}});
  {
    // Insert a new element. Check that it succeeds and that the returned iterator points to the new
    // element.
    const auto& ret = map.insert_or_assign("four", 4);
    EXPECT_TRUE(ret.second);
    const auto& it = ret.first;
    EXPECT_EQ(it->first, "four");
    EXPECT_EQ(it->second, 4);
    // This is a new element, so it should be at the end of the map.
    EXPECT_EQ(std::distance(std::begin(map), it),
              std::distance(std::begin(map), std::end(map)) - 1);
  }
  {
    // Attempt to insert and element with a key that already exists. Check the return value and that
    // the new value is correct.
    const auto& ret = map.insert_or_assign("two", 20);
    EXPECT_FALSE(ret.second);
    const auto& it = ret.first;
    EXPECT_EQ(it->first, "two");
    EXPECT_EQ(it->second, 20);
    // This is an existing element, so it should be at the same position in the map.
    EXPECT_EQ(std::distance(std::begin(map), it) + 1, 2);
  }
  {
    const std::string new_key = "five";
    const int new_value = 5;
    const auto& ret = map.insert_or_assign(new_key, new_value);
    EXPECT_TRUE(ret.second);
    const auto& it = ret.first;
    EXPECT_EQ(it->first, new_key);
    EXPECT_EQ(it->second, new_value);
    // This is a new element, so it should be at the end of the map.
    EXPECT_EQ(std::distance(std::begin(map), it),
              std::distance(std::begin(map), std::end(map)) - 1);
  }
}

TEST(OrderedMap, emplace) {
  // Test the emplace operator that it works as expected.
  OMap map;

  {
    const auto& ret = map.emplace("one", 1);
    EXPECT_TRUE(ret.second);
    const auto& it = ret.first;
    EXPECT_EQ(it->first, "one");
    EXPECT_EQ(it->second, 1);
    // This is a new element, so it should be at the end of the map.
    EXPECT_EQ(std::distance(std::begin(map), it),
              std::distance(std::begin(map), std::end(map)) - 1);
  }
  {
    // Try to emplace an element with the same key.
    const auto& ret = map.emplace("one", 10);
    EXPECT_FALSE(ret.second);
    const auto& it = ret.first;
    EXPECT_EQ(it->first, "one");
    EXPECT_EQ(it->second, 1);
  }
  {
    // Try to emplace an element with a different key.
    const auto& ret = map.emplace(std::string("two"), static_cast<int16_t>(2));
    EXPECT_TRUE(ret.second);
    const auto& it = ret.first;
    EXPECT_EQ(it->first, "two");
    EXPECT_EQ(it->second, 2);
    // This is a new element, so it should be at the end of the map.
    EXPECT_EQ(std::distance(std::begin(map), it),
              std::distance(std::begin(map), std::end(map)) - 1);
  }
}

// TEST(OrderedMap, emplace_hint) {}

// TEST(OrderedMap, try_emplace) {}

TEST(OrderedMap, erase) {
  OMap map({{"one", 1}, {"two", 2}, {"three", 3}});
  EXPECT_EQ(map.size(), 3);

  // Erase an element using an iterator
  const auto& it = map.erase(std::begin(map));
  EXPECT_EQ(it->first, "two");
  EXPECT_EQ(it->second, 2);
  EXPECT_EQ(std::distance(std::begin(map), it), 0);
  EXPECT_EQ(map.size(), 2);

  // Erase an element using a const iterator
  const auto& cit = map.erase(std::cbegin(map));
  EXPECT_EQ(cit->first, "three");
  EXPECT_EQ(cit->second, 3);
  EXPECT_EQ(std::distance(std::begin(map), cit), 0);
  EXPECT_EQ(map.size(), 1);

  // Erase an element using a key
  const auto& count = map.erase("three");
  EXPECT_EQ(count, 1);
  EXPECT_EQ(map.size(), 0);

  // Try erasing an element using a key (but map is empty)
  const auto& count2 = map.erase("foo");
  EXPECT_EQ(count2, 0);
}

TEST(OrderedMap, size_t_test) {
  // This test ensures that we can create an ordered_map that uses the same type for the key_type
  // and value_type and that when using a size_t all of the functions work. With some of the
  // initiali implementation the iterator overload resolution was failing.
  flexi_cfg::details::ordered_map<size_t, size_t> map = {{0, 3}, {1, 2}, {2, 1}, {3, 0}};
  EXPECT_EQ(map.size(), 4);

  for (auto it = std::begin(map); it != std::end(map); ++it) {
    EXPECT_EQ(it->first, 3 - it->second);
  }
}
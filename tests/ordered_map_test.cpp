#include "flexi_cfg/details/ordered_map.h"

#include <fmt/format.h>
#include <fmt/ranges.h>
#include <gtest/gtest.h>

#include <iterator>  // For iterator concepts and traits
#include <range/v3/view/map.hpp>
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

TEST(OrderedMap, ConstructorsExtended) {
  using CompareType = std::equal_to<>;
  using TestMap = flexi_cfg::details::ordered_map<int, std::string, std::hash<int>, CompareType>;
  using ValueType = TestMap::value_type;
  using AllocType = std::allocator<ValueType>;

  AllocType myAlloc;
  CompareType myComp;

  // explicit ordered_map(const Compare& comp, const Allocator& alloc = Allocator())
  TestMap map1(myComp, myAlloc);
  EXPECT_TRUE(map1.empty());
  EXPECT_EQ(map1.get_allocator(), myAlloc);

  // explicit ordered_map(const Allocator& alloc)
  TestMap map2(myAlloc);
  EXPECT_TRUE(map2.empty());
  EXPECT_EQ(map2.get_allocator(), myAlloc);

  // template< class InputIt > ordered_map(InputIt first, InputIt last, const Compare& comp, const
  // Allocator& alloc = Allocator())
  std::vector<ValueType> data_vec = {{1, "one"}, {2, "two"}};
  TestMap map3(data_vec.begin(), data_vec.end(), myComp, myAlloc);
  EXPECT_EQ(map3.size(), 2);
  EXPECT_EQ(map3.get_allocator(), myAlloc);
  EXPECT_EQ(map3.at(1), "one");
  EXPECT_EQ(map3.at(2), "two");
  // Check order
  auto it3 = map3.begin();
  EXPECT_EQ(it3->first, 1);
  ++it3;
  EXPECT_EQ(it3->first, 2);

  // ordered_map(std::initializer_list<value_type> init, const Compare& comp, const Allocator& alloc
  // = Allocator())
  TestMap map4({{3, "three"}, {4, "four"}}, myComp, myAlloc);
  EXPECT_EQ(map4.size(), 2);
  EXPECT_EQ(map4.get_allocator(), myAlloc);
  EXPECT_EQ(map4.at(3), "three");
  EXPECT_EQ(map4.at(4), "four");
  // Check order
  auto it4 = map4.begin();
  EXPECT_EQ(it4->first, 3);
  ++it4;
  EXPECT_EQ(it4->first, 4);

  // Test with default allocator for Compare-only constructor
  TestMap map5(myComp);
  EXPECT_TRUE(map5.empty());
}

TEST(OrderedMap, Capacity) {
  OMap map;
  EXPECT_TRUE(map.empty());
  EXPECT_EQ(map.size(), 0);
  EXPECT_GT(map.max_size(), 0);

  map.insert({"one", 1});
  EXPECT_FALSE(map.empty());
  EXPECT_EQ(map.size(), 1);

  map.insert({"two", 2});
  EXPECT_FALSE(map.empty());
  EXPECT_EQ(map.size(), 2);

  map.clear();
  EXPECT_TRUE(map.empty());
  EXPECT_EQ(map.size(), 0);
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
  {
    // Iterator conversion!
    OMap map = build_map(expected_keys);

    OMap::iterator it_begin = map.begin();
    OMap::iterator it_end = map.end();

    // Test explicit conversion
    EXPECT_EQ(OMap::const_iterator(it_begin), map.cbegin());
    EXPECT_EQ(OMap::const_iterator(it_end), map.cend());

    // Test implicit conversion via assignment
    OMap::const_iterator cit_assigned_begin = it_begin;
    EXPECT_EQ(cit_assigned_begin, map.cbegin());

    OMap::const_iterator cit_assigned_end = it_end;
    EXPECT_EQ(cit_assigned_end, map.cend());
  }

  {
    OMap map = build_map(expected_keys);

    EXPECT_EQ(std::begin(map), map.begin());
    EXPECT_EQ(std::end(map), map.end());

    EXPECT_EQ(std::cbegin(map), map.cbegin());
    EXPECT_EQ(std::cend(map), map.cend());

    auto check = [expected_keys](auto it, auto expected_idx, auto begin) {
      EXPECT_EQ(it, std::next(begin, expected_idx));
      EXPECT_EQ(std::distance(begin, it), expected_idx);
      EXPECT_EQ(it->second, expected_idx);
      EXPECT_EQ(it->first, expected_keys[expected_idx]);
    };

    auto run_test = [expected_keys, &map](auto it, auto check_) {
      // Check iterator addition/subtraction:
      check_(it, 0);

      // Increment iterator:
      it++;
      check_(it, 1);

      it += 1;
      check_(it, 2);

      it = it + 2;
      check_(it, 4);

      ++it;
      check_(it, 5);

      --it;
      check_(it, 4);

      it = it - 1;
      check_(it, 3);

      it -= 2;
      check_(it, 1);

      it--;
      check_(it, 0);
    };

    run_test(std::begin(map), [check, expected_keys, &map](auto it, auto expected_idx) {
      return check(it, expected_idx, std::begin(map));
    });
    run_test(std::cbegin(map), [check, expected_keys, &map](auto it, auto expected_idx) {
      return check(it, expected_idx, std::cbegin(map));
    });
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

TEST(OrderedMap, InsertOrAssign) {
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

// TEST(OrderedMap, swap) {}

TEST(OrderedMap, extract) {
  SCOPED_TRACE("Testing extract");
  {
    SCOPED_TRACE("Testing key version");
    // Test "key" version
    OMap map({{"one", 1}, {"two", 2}, {"three", 3}, {"bar", 4}, {"baz", 5}});
    EXPECT_EQ(map.size(), 5);

    {
      const auto node = map.extract("two");
      EXPECT_EQ(node.key(), "two");
      EXPECT_EQ(node.mapped(), 2);
      EXPECT_EQ(map.size(), 4);
      EXPECT_FALSE(map.contains("two"));
    }
    {
      // Extract another key that is past the previously extracted key
      const auto node = map.extract("bar");
      EXPECT_EQ(node.key(), "bar");
      EXPECT_EQ(node.mapped(), 4);
      EXPECT_EQ(map.size(), 3);
      EXPECT_FALSE(map.contains("bar"));
    }

    // Try extracting a key that doesn't exist
    const auto node2 = map.extract("foo");
    EXPECT_TRUE(node2.empty());
  }
  {
    SCOPED_TRACE("Testing iterator version");
    // Test "iterator" version
    OMap map({{"one", 1}, {"two", 2}, {"three", 3}, {"bar", 4}});
    EXPECT_EQ(map.size(), 4);
    {
      const auto it = map.find("two");
      const auto node = map.extract(it);
      EXPECT_EQ(node.key(), "two");
      EXPECT_EQ(node.mapped(), 2);
      EXPECT_EQ(map.size(), 3);
      EXPECT_FALSE(map.contains("two"));
    }
    {
      const auto it = map.find("bar");
      const auto node = map.extract(it);
      EXPECT_EQ(node.key(), "bar");
      EXPECT_EQ(node.mapped(), 4);
      EXPECT_EQ(map.size(), 2);
      EXPECT_FALSE(map.contains("bar"));
    }
    // Try extracting an iterator that doesn't exist
    auto it2 = map.find("foo");
    const auto node2 = map.extract(it2);
    EXPECT_TRUE(node2.empty());
  }
}

TEST(OrderedMap, merge) {
  // Verify that merge works as expected and that the resulting map is ordered as expected.
  {
    OMap map1({{"one", 0}, {"two", 1}, {"three", 2}});
    OMap map2({{"four", 3}, {"five", 4}, {"six", 5}});
    map1.merge(map2);
    EXPECT_EQ(map1.size(), 6);
    EXPECT_EQ(map2.size(), 0);
    EXPECT_TRUE(map2.empty());
    EXPECT_EQ(map1.at("one"), 0);
    EXPECT_EQ(map1.at("two"), 1);
    EXPECT_EQ(map1.at("three"), 2);
    EXPECT_EQ(map1.at("four"), 3);
    EXPECT_EQ(map1.at("five"), 4);
    EXPECT_EQ(map1.at("six"), 5);

    std::vector<std::string> expected_keys = {"one", "two", "three", "four", "five", "six"};
    for (const auto& [key, idx] : map1) {
      EXPECT_EQ(key, expected_keys[idx]);
    }
  }
  {
    OMap map1({{"one", 0}, {"two", 1}, {"three", 2}});
    OMap map2({{"four", 3}, {"two", 0}, {"one", 1}});
    map1.merge(map2);
    EXPECT_EQ(map1.size(), 4);
    EXPECT_EQ(map2.size(), 2);

    {
      std::vector<std::string> expected_keys = {"one", "two", "three", "four"};
      for (const auto& [key, idx] : map1) {
        EXPECT_EQ(key, expected_keys[idx]);
      }
    }
    {
      std::vector<std::string> expected_keys = {"two", "one"};
      for (const auto& [key, idx] : map2) {
        EXPECT_EQ(key, expected_keys[idx]);
      }
    }
  }
}

TEST(OrderedMap, at) {
  OMap map({{"one", 1}, {"two", 2}, {"three", 3}, {"one", 4}});
  EXPECT_EQ(map.at("one"), 1);
  EXPECT_EQ(map.at("two"), 2);
  EXPECT_EQ(map.at("three"), 3);
  EXPECT_THROW(map.at("four"), std::out_of_range);

  // Test const version of at()
  const OMap cmap_at = map;  // Create a const copy from existing map
  EXPECT_EQ(cmap_at.at("one"), 1);
  EXPECT_EQ(cmap_at.at("two"), 2);
  EXPECT_EQ(cmap_at.at("three"), 3);
  EXPECT_THROW(cmap_at.at("four"), std::out_of_range);
}

TEST(OrderedMap, OperatorSquareBracketsAccess) {
  OMap map({{"one", 1}, {"two", 2}, {"three", 3}, {"two", 4}});
  EXPECT_EQ(map["one"], 1);
  EXPECT_EQ(map["two"], 2);
  EXPECT_EQ(map["three"], 3);
  // This passes. Because "four" doesn't already exist in the map, it is inserted and the value is
  // zero initialized.
  // This is the same behavior as std::map.
  EXPECT_EQ(map["four"], 0);
}

TEST(OrderedMap, OperatorSquareBracketsInsertion) {
  OMap map({{"one", 1}, {"two", 2}});
  EXPECT_EQ(map["one"], 1);
  EXPECT_EQ(map["two"], 2);
  map["three"] = 3;
  EXPECT_EQ(map["three"], 3);
  EXPECT_EQ(map.size(), 3);
  EXPECT_EQ(map.at("three"), 3);

  // Verify order after operator[] insertions
  auto it = map.begin();
  EXPECT_EQ(it->first, "one");
  ++it;
  EXPECT_EQ(it->first, "two");
  ++it;
  EXPECT_EQ(it->first, "three");
  ++it;
  EXPECT_EQ(it, map.end());

  // Test operator[](Key&&)
  OMap map_rval;
  map_rval[std::string("alpha")] = 10;
  EXPECT_EQ(map_rval.at("alpha"), 10);
  EXPECT_EQ(map_rval.size(), 1);

  map_rval[std::string("beta")] = 20;
  map_rval[std::string("gamma")] = 30;
  EXPECT_EQ(map_rval.at("beta"), 20);
  EXPECT_EQ(map_rval.at("gamma"), 30);
  EXPECT_EQ(map_rval.size(), 3);

  auto it_rval = map_rval.begin();
  EXPECT_EQ(it_rval->first, "alpha");
  ++it_rval;
  EXPECT_EQ(it_rval->first, "beta");
  ++it_rval;
  EXPECT_EQ(it_rval->first, "gamma");
  ++it_rval;
  EXPECT_EQ(it_rval, map_rval.end());

  // Access existing with Key&&
  EXPECT_EQ(map_rval[std::string("alpha")], 10);
}

TEST(OrderedMap, count) {
  OMap map({{"one", 1}, {"two", 2}, {"three", 3}});
  EXPECT_EQ(map.count("one"), 1);
  EXPECT_EQ(map.count("two"), 1);
  EXPECT_EQ(map.count("three"), 1);
  EXPECT_EQ(map.count("four"), 0);

  OMap map2({{"one", 1}, {"two", 2}, {"three", 3}, {"one", 4}});
  EXPECT_EQ(map2.count("one"), 1);
  EXPECT_EQ(map2.count("two"), 1);
  EXPECT_EQ(map2.count("three"), 1);
  EXPECT_EQ(map2.count("four"), 0);
}

TEST(OrderedMap, find) {
  OMap map = {{"this", 0}, {"is", 1},  {"a", 2},      {"test", 3}, {"to", 4},
              {"see", 5},  {"how", 6}, {"things", 7}, {"work", 8}};
  {
    const auto it = map.find("see");
    EXPECT_EQ(it->first, "see");
    EXPECT_EQ(it->second, 5);
  }
  {
    const auto it = map.find("doesn't exist");
    EXPECT_EQ(it, std::end(map));
  }

  const auto c_map(map);
  {
    const auto it = c_map.find("see");
    EXPECT_EQ(it->first, "see");
    EXPECT_EQ(it->second, 5);
  }
  {
    const auto it = c_map.find("doesn't exist");
    EXPECT_EQ(it, std::cend(c_map));
  }
}

TEST(OrderedMap, SizeTTest) {
  // This test ensures that we can create an ordered_map that uses the same type for the key_type
  // and value_type and that when using a size_t all of the functions work. With some of the
  // initial implementation the iterator overload resolution was failing.
  flexi_cfg::details::ordered_map<size_t, size_t> map = {{0, 3}, {1, 2}, {2, 1}, {3, 0}};
  EXPECT_EQ(map.size(), 4);

  for (auto it = std::begin(map); it != std::end(map); ++it) {
    EXPECT_EQ(it->first, 3 - it->second);
  }
}

TEST(OrderedMap, Ranges) {
  // Iterator concepts for OMap::iterator
  static_assert(std::input_iterator<OMap::iterator>, "OMap::iterator should be an input_iterator");
  static_assert(std::forward_iterator<OMap::iterator>,
                "OMap::iterator should be a forward_iterator");
  static_assert(std::bidirectional_iterator<OMap::iterator>,
                "OMap::iterator should be a bidirectional_iterator");

  // Iterator concepts for OMap::const_iterator
  static_assert(std::input_iterator<OMap::const_iterator>,
                "OMap::const_iterator should be an input_iterator");
  static_assert(std::forward_iterator<OMap::const_iterator>,
                "OMap::const_iterator should be a forward_iterator");
  static_assert(std::bidirectional_iterator<OMap::const_iterator>,
                "OMap::const_iterator should be a bidirectional_iterator");

  // Iterator concepts for OMap::reverse_iterator
  static_assert(std::bidirectional_iterator<OMap::reverse_iterator>,
                "OMap::reverse_iterator should be a bidirectional_iterator");

  // Iterator concepts for OMap::const_reverse_iterator
  static_assert(std::bidirectional_iterator<OMap::const_reverse_iterator>,
                "OMap::const_reverse_iterator should be a bidirectional_iterator");

  // Range concepts for OMap
  static_assert(std::ranges::range<OMap>, "OMap should be a range");
  static_assert(std::ranges::forward_range<OMap>, "OMap should be a forward_range");
  static_assert(std::ranges::bidirectional_range<OMap>, "OMap should be a bidirectional_range");

  // Range concepts for const OMap
  static_assert(std::ranges::range<const OMap>, "const OMap should be a range");
  static_assert(std::ranges::forward_range<const OMap>, "const OMap should be a forward_range");
  static_assert(std::ranges::bidirectional_range<const OMap>,
                "const OMap should be a bidirectional_range");

  OMap map = {{"this", 0}, {"is", 1},  {"a", 2},      {"test", 3}, {"to", 4},
              {"see", 5},  {"how", 6}, {"things", 7}, {"work", 8}};

  // Test basic range operations
  auto b_it = std::begin(map);
  auto e_it = std::end(map);
  EXPECT_NE(b_it, e_it);
  EXPECT_EQ(std::distance(b_it, e_it), map.size());

  const auto cb_it = std::cbegin(map);
  const auto ce_it = std::cend(map);
  EXPECT_NE(cb_it, ce_it);
  EXPECT_EQ(std::distance(cb_it, ce_it), map.size());

  auto size = std::size(map);
  EXPECT_EQ(size, 9);

  auto empty = std::empty(map);
  EXPECT_FALSE(empty);

  // Test range-based for loop
  {
    std::vector<std::string> keys;
    std::vector<int> values;
    for (const auto& [key, value] : map) {
      keys.push_back(key);
      values.push_back(value);
    }
    EXPECT_EQ(keys.size(), map.size());
    EXPECT_EQ(values.size(), map.size());
    EXPECT_EQ(keys[0], "this");
    EXPECT_EQ(values[0], 0);
    EXPECT_EQ(keys.back(), "work");
    EXPECT_EQ(values.back(), 8);
  }

  // Test reverse iteration
  {
    std::vector<std::string> rev_keys;
    for (auto it = map.rbegin(); it != map.rend(); ++it) {
      rev_keys.push_back(it->first);
    }
    EXPECT_EQ(rev_keys.size(), map.size());
    EXPECT_EQ(rev_keys[0], "work");
    EXPECT_EQ(rev_keys.back(), "this");
  }

  // Test range adaptors
  {
    // Test keys view
    auto keys = ranges::views::keys(map);
    std::vector<std::string> key_vec(keys.begin(), keys.end());
    EXPECT_EQ(key_vec.size(), map.size());
    EXPECT_EQ(key_vec[0], "this");
    EXPECT_EQ(key_vec.back(), "work");

    // Test values view
    auto values = ranges::views::values(map);
    std::vector<int> value_vec(values.begin(), values.end());
    EXPECT_EQ(value_vec.size(), map.size());
    EXPECT_EQ(value_vec[0], 0);
    EXPECT_EQ(value_vec.back(), 8);

    // Test transform view
    auto transformed = ranges::views::transform(map, [](const auto& pair) {
      return std::make_pair(pair.first + "_transformed", pair.second * 2);
    });
    std::vector<std::pair<std::string, int>> trans_vec(transformed.begin(), transformed.end());
    EXPECT_EQ(trans_vec.size(), map.size());
    EXPECT_EQ(trans_vec[0].first, "this_transformed");
    EXPECT_EQ(trans_vec[0].second, 0);
    EXPECT_EQ(trans_vec.back().first, "work_transformed");
    EXPECT_EQ(trans_vec.back().second, 16);
  }

  // Test const range operations
  const OMap& const_map = map;
  {
    std::vector<std::string> const_keys;
    for (const auto& [key, value] : const_map) {
      const_keys.push_back(key);
    }
    EXPECT_EQ(const_keys.size(), const_map.size());
    EXPECT_EQ(const_keys[0], "this");
    EXPECT_EQ(const_keys.back(), "work");
  }

  // Test range algorithms
  {
    auto it =
        std::find_if(map.begin(), map.end(), [](const auto& pair) { return pair.second == 5; });
    EXPECT_NE(it, map.end());
    EXPECT_EQ(it->first, "see");
    EXPECT_EQ(it->second, 5);

    auto count = std::count_if(map.begin(), map.end(),
                               [](const auto& pair) { return pair.second % 2 == 0; });
    EXPECT_EQ(count, 5);  // 0, 2, 4, 6, 8 are even
  }

  fmt::print("Keys: {}\n", fmt::join(ranges::views::keys(map), ", "));
  fmt::print("Values: {}\n", fmt::join(ranges::views::values(map), ", "));
}

TEST(OrderedMap, TransparentLookup) {
  // Test transparent lookup with different but comparable types
  OMap map = {{"one", 1}, {"two", 2}};

  // Test with string_view (converting to string)
  std::string_view sv = "one";
  EXPECT_EQ(map.count(std::string(sv)), 1);
  EXPECT_TRUE(map.contains(std::string(sv)));
  EXPECT_NE(map.find(std::string(sv)), map.end());

  // Test with const char* (converting to string)
  EXPECT_EQ(map.count(std::string("two")), 1);
  EXPECT_TRUE(map.contains(std::string("two")));
  EXPECT_NE(map.find(std::string("two")), map.end());
}

TEST(OrderedMap, NodeHandle) {
  OMap map1 = {{"one", 1}, {"two", 2}};
  OMap map2;

  // Test node extraction and insertion
  auto node = map1.extract("one");
  EXPECT_FALSE(node.empty());
  EXPECT_EQ(node.key(), "one");
  EXPECT_EQ(node.mapped(), 1);

  // Test inserting extracted node
  auto [it, inserted, nh] = map2.insert(std::move(node));
  EXPECT_TRUE(inserted);
  EXPECT_TRUE(nh.empty());
  EXPECT_EQ(map2["one"], 1);

  // Test empty node
  auto empty_node = map1.extract("nonexistent");
  EXPECT_TRUE(empty_node.empty());
}

TEST(OrderedMap, EqualRange) {
  OMap map = {{"one", 1}, {"two", 2}, {"three", 3}};

  // Test existing key
  auto [it1, it2] = map.equal_range("two");
  EXPECT_NE(it1, it2);
  EXPECT_EQ(it1->first, "two");
  EXPECT_EQ(it1->second, 2);
  EXPECT_EQ(it2->first, "three");

  // Test non-existing key
  auto [it3, it4] = map.equal_range("four");
  EXPECT_EQ(it3, map.end());
  EXPECT_EQ(it4, map.end());
}

TEST(OrderedMap, Bounds) {
  OMap map = {{"one", 1}, {"two", 2}, {"three", 3}};

  // Test lower_bound
  auto lb = map.lower_bound("two");
  EXPECT_EQ(lb->first, "two");

  // Test upper_bound
  auto ub = map.upper_bound("two");
  EXPECT_EQ(ub->first, "three");

  // Test non-existing key
  auto lb2 = map.lower_bound("four");
  EXPECT_EQ(lb2, map.end());
  auto ub2 = map.upper_bound("four");
  EXPECT_EQ(ub2, map.end());
}

TEST(OrderedMap, ExceptionSafety) {
  OMap map = {{"one", 1}, {"two", 2}};

  // Test exception safety of insert
  try {
    map.insert({"three", 3});
    EXPECT_TRUE(map.contains("three"));
  } catch (...) {
    EXPECT_FALSE(map.contains("three"));
  }

  // Test exception safety of operator[]
  try {
    map["four"] = 4;
    EXPECT_TRUE(map.contains("four"));
  } catch (...) {
    EXPECT_FALSE(map.contains("four"));
  }
}

TEST(OrderedMap, Allocator) {
  using CustomAlloc = std::allocator<std::pair<const std::string, int>>;
  flexi_cfg::details::ordered_map<std::string, int, std::hash<std::string>, std::equal_to<>,
                                  CustomAlloc>
      map;

  // Test allocator propagation
  EXPECT_EQ(map.get_allocator(), CustomAlloc());

  // Test allocator-aware operations
  map.insert({"one", 1});
  EXPECT_EQ(map["one"], 1);
}

TEST(OrderedMap, Compare) {
  using CustomCompare = std::less<std::string>;
  flexi_cfg::details::ordered_map<std::string, int, std::hash<std::string>, CustomCompare> map;

  // Test key comparison
  auto key_comp = map.key_comp();
  EXPECT_TRUE(key_comp("a", "b"));

  // Test value comparison
  auto value_comp = map.value_comp();
  EXPECT_TRUE(value_comp({"a", 1}, {"b", 2}));
}

TEST(OrderedMap, EdgeCases) {
  OMap map;

  // Test empty map operations
  EXPECT_TRUE(map.empty());
  EXPECT_EQ(map.size(), 0);
  EXPECT_EQ(map.find("nonexistent"), map.end());

  // Test self-assignment
  map = {{"one", 1}};
  map = map;
  EXPECT_EQ(map["one"], 1);

  // Test self-merge
  map.merge(map);
  EXPECT_EQ(map.size(), 1);

  // Test iterator invalidation
  map = {{"one", 1}, {"two", 2}, {"three", 3}};
  auto it = map.begin();
  map.erase(it);
  // The iterator should be invalidated, but we can't test that directly
  // Instead, we can verify the map state
  EXPECT_EQ(map.size(), 2);
  EXPECT_FALSE(map.contains("one"));

  map = {{"two", 2}, {"four", 4}, {"six", 6}};
  it = map.begin() + 1;
  map.erase(it);
  EXPECT_EQ(map.size(), 2);
  EXPECT_FALSE(map.contains("four"));
}

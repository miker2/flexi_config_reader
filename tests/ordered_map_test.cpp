#include <gtest/gtest.h>

#include <fmt/format.h>

#include <string>

#include "flexi_cfg/details/ordered_map.h"
#include "flexi_cfg/utils.h"

namespace {
using MyMap = std::unordered_map<std::string, int>;
using OMap = flexi_cfg::details::ordered_map<std::string, int>;
}

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
}

TEST(OrderedMap, Test) {
    OMap map = {
        {"this", 0},
        {"is", 1},
        {"a", 2},
        {"test", 3},
        {"to", 4},
        {"see", 5},
        {"how", 6},
        {"things", 7},
        {"work", 8}
    };

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

    { // Insertion test. Need to test the other signatures of this call.
        auto ret = map.insert(OMap::value_type{"new 1", -1});
        fmt::print("success: {}, it: {}\n", ret.second, *ret.first);
        auto fail = map.insert(OMap::value_type{"test", -1});
        fmt::print("Success: {}, it: {}\n", fail.second, *fail.first);
        auto ret2 = map.insert(OMap::value_type{"see", -1});
        fmt::print("success: {}, it: {}\n", ret2.second, *ret2.first);

    }




    {
        MyMap test = { {"a", 0}, {"b", 1}, {"c", 2} };
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
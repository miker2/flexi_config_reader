#pragma once

#include <string>

namespace flexi_cfg::visitor {

template <typename Visitor>
concept KeyVisitor = requires(Visitor& visitor, std::string key) { visitor.onKey(key); };

template <typename Visitor>
concept BoolValueVisitor = KeyVisitor<Visitor> && requires(Visitor& visitor, bool value) {
  { visitor.onValue(value) };
};

template <typename Visitor>
concept IntValueVisitor =
    KeyVisitor<Visitor> && requires(Visitor& visitor, int64_t value, uint64_t unsigned_value) {
      { visitor.onValue(value) };
      { visitor.onValue(unsigned_value) };
    };

template <typename Visitor>
concept FloatValueVisitor = KeyVisitor<Visitor> && requires(Visitor& visitor, double value) {
  { visitor.onValue(value) };
};

template <typename Visitor>
concept StringValueVisitor = KeyVisitor<Visitor> && requires(Visitor& visitor, std::string value) {
  { visitor.onValue(value) };
};

template <typename Visitor>
concept ListVisitor = requires(Visitor& visitor) {
  { visitor.beginList() };
  { visitor.endList() };
};

template <typename Visitor>
concept StructVisitor = KeyVisitor<Visitor> && requires(Visitor& visitor) {
  { visitor.beginStruct() };
  { visitor.endStruct() };
};

template <typename Visitor>
concept TypedVisitor = KeyVisitor<Visitor> || IntValueVisitor<Visitor> ||
                       FloatValueVisitor<Visitor> || StringValueVisitor<Visitor> ||
                       BoolValueVisitor<Visitor> || ListVisitor<Visitor> || StructVisitor<Visitor>;

}  // namespace flexi_cfg::visitor

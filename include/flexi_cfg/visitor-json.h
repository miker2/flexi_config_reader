#pragma once

#include <fmt/format.h>

#include <string>

#include "flexi_cfg/visitor.h"

namespace flexi_cfg::visitor {

class JsonVisitor {
 public:
  operator std::string() { return json_; }
  void onKey(std::string key) { json_ += fmt::format("\"{}\":", key); }
  void onValue(std::string value) { json_ += fmt::format("\"{}\",", value); }
  void onValue(int64_t value) { json_ += fmt::format("{},", value); }
  void onValue(uint64_t value) { json_ += fmt::format("{},", value); }
  void onValue(double value) { json_ += fmt::format("{},", value); }
  void onValue(bool value) { json_ += fmt::format("{},", value); }
  void beginStruct() {
    json_ += "{";
    struct_count_++;
  }
  void endStruct() {
    stripComma();
    struct_count_--;
    json_ += "}";
    if (struct_count_ > 0) {
      json_ += ",";
    }
  }
  void beginList() { json_ += "["; }
  void endList() {
    stripComma();
    json_ += "],";
  }

 private:
  void stripComma() {
    if (json_.back() == ',') {
      json_.pop_back();  // strip last ','
    }
  }
  std::string json_;
  unsigned int struct_count_{};
};

static_assert(StructVisitor<JsonVisitor>);
static_assert(ListVisitor<JsonVisitor>);
static_assert(StringValueVisitor<JsonVisitor>);
static_assert(BoolValueVisitor<JsonVisitor>);
static_assert(IntValueVisitor<JsonVisitor>);
static_assert(FloatValueVisitor<JsonVisitor>);

class PrettyJsonVisitor {
 public:
  operator std::string() { return json_; }
  void onKey(std::string key) {
    updateIndent();
    json_ += fmt::format("{}\"{}\" :", indent_, key);
    indent_ = " ";  // inline value
  }
  void onValue(std::string value) { json_ += fmt::format("{}\"{}\",\n", indent_, value); }
  void onValue(int64_t value) { json_ += fmt::format("{}{},\n", indent_, value); }
  void onValue(uint64_t value) { json_ += fmt::format("{}{},\n", indent_, value); }
  void onValue(double value) { json_ += fmt::format("{}{},\n", indent_, value); }
  void onValue(bool value) { json_ += fmt::format("{}{},\n", indent_, value); }
  void beginStruct() {
    json_ += fmt::format("{}{{\n", indent_);  // open inline
    nest_count_++;
    updateIndent();
  }
  void endStruct() {
    stripComma();
    nest_count_--;
    updateIndent();
    json_ += fmt::format("{}}}", indent_);  // close at parent indent
    if (nest_count_ > 0) {
      json_ += ",\n";
    } else {
      json_ += "\n";
    }
  }
  void beginList() {
    json_ += fmt::format("{}[\n", indent_);  // open inline
    nest_count_++;
    updateIndent();
  }
  void endList() {
    stripComma();
    nest_count_--;
    updateIndent();
    json_ += fmt::format("{}],\n", indent_);  // close at parent indent
  }

 private:
  void updateIndent() { indent_ = std::string(nest_count_ * 2, ' '); }
  void stripComma() {
    if (json_.ends_with(",\n")) {  // replace ',\n' with '\n'
      json_.pop_back();
      json_.pop_back();
      json_.push_back('\n');
    }
  }
  std::string indent_{};
  std::string json_;
  unsigned int nest_count_{};
};

static_assert(StructVisitor<PrettyJsonVisitor>);
static_assert(ListVisitor<PrettyJsonVisitor>);
static_assert(StringValueVisitor<PrettyJsonVisitor>);
static_assert(BoolValueVisitor<PrettyJsonVisitor>);
static_assert(IntValueVisitor<PrettyJsonVisitor>);
static_assert(FloatValueVisitor<PrettyJsonVisitor>);

}  // namespace flexi_cfg::visitor

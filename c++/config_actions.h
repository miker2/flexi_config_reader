#pragma once

#include <exception>
#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>
#include <vector>

#include "config_classes.h"
#include "config_grammar.h"
#include "config_helpers.h"

#define VERBOSE_DEBUG 0

namespace config {

enum class GroupType { kNone = 0, kStruct, kProto, kReference };

const std::string DEFAULT_RES{"***"};

// Add action to perform when a `proto` is encountered!
struct ActionData {
  std::vector<std::string> proto_names{};
  std::vector<std::string> struct_names{};
  std::vector<std::string> reference_names{};

  int depth{0};  // Nesting level
  GroupType type{GroupType::kNone};
  GroupType type_outer{GroupType::kNone};

  std::string result = DEFAULT_RES;
  std::string var = DEFAULT_RES;
  std::vector<std::string> keys;
  std::vector<std::string> flat_keys;

  std::vector<std::pair<std::string, std::string>> pairs;
  std::vector<std::pair<std::string, std::string>> special_pairs;

  // NOTE: A struct, is just a fancy way of describing a key, that contains an array of key/value
  // pairs (much like a json object). We want to be able to represent both a struct and a simple
  // key/value pair, so the top level can't be a struct, but must be more generically representable.

  // Nominally, we'd have just a `types::ConfigBase` at the very root (which we could still do,
  // maybe), but we need to be able to represent an array of items here in order to support
  // duplicate structs, etc. We'll join these all up later.
  std::vector<std::map<std::string, std::shared_ptr<types::ConfigBase>>> cfg_res{1};
  std::shared_ptr<types::ConfigBase> obj_res;
  std::vector<std::shared_ptr<types::ConfigStructLike>> objects;

  void print() {
    std::cout << "Proto names: \n";
    for (const auto& pn : proto_names) {
      std::cout << "  " << pn << "\n";
    }
    std::cout << "Struct names: \n";
    for (const auto& pn : struct_names) {
      std::cout << "  " << pn << "\n";
    }
    std::cout << "Reference names: \n";
    for (const auto& pn : reference_names) {
      std::cout << "  " << pn << "\n";
    }
    std::cout << "Keys: \n";
    for (const auto& key : keys) {
      std::cout << "  " << key << "\n";
    }
    std::cout << "Flat Keys: \n";
    for (const auto& key : flat_keys) {
      std::cout << "  " << key << "\n";
    }
    std::cout << "Pairs: \n";
    for (const auto& pair : pairs) {
      std::cout << "  " << pair.first << " = " << pair.second << "\n";
    }
    std::cout << "Special Pairs: \n";
    for (const auto& pair : special_pairs) {
      std::cout << "  " << pair.first << " = " << pair.second << "\n";
    }
    std::cout << "result: " << result << std::endl;
    std::cout << "var: " << var << std::endl;
    std::cout << "==========" << std::endl;
    std::cout << "cfg_res: " << std::endl;
    for (const auto& mp : cfg_res) {
      for (const auto& kv : mp) {
        std::cout << kv.first << " = " << kv.second << std::endl;
      }
    }
    if (obj_res != nullptr) {
      std::cout << "obj_res: " << obj_res << std::endl;
    } else {
      std::cout << "obj_res: "
                << "empty" << std::endl;
    }
    std::cout << "objects: " << std::endl;
    for (const auto& obj : objects) {
      std::cout << obj << std::endl;
    }
    std::cout << "^^^^^^^^^^" << std::endl;
  }
};

template <typename Rule>
struct action : peg::nothing<Rule> {};

template <>
struct action<KEY> {
  template <typename ActionInput>
  static void apply(const ActionInput& in, ActionData& out) {
#if VERBOSE_DEBUG
    std::cout << std::string(out.depth * 2, ' ') << "Found key: '" << in.string() << "'"
              << std::endl;
#endif
    out.keys.emplace_back(in.string());
  }
};

template <>
struct action<VALUE> {
  template <typename ActionInput>
  static void apply(const ActionInput& in, ActionData& out) {
#if VERBOSE_DEBUG
    std::cout << std::string(out.depth * 2, ' ') << "Found value: " << in.string() << std::endl;
#endif
    out.result = in.string();
    out.obj_res = std::make_shared<types::ConfigValue>(in.string());
  }
};

template <>
struct action<VAR> {
  template <typename ActionInput>
  static void apply(const ActionInput& in, ActionData& out) {
    std::cout << std::string(out.depth * 2, ' ') << "Found var: " << in.string() << std::endl;
    // Store in both "result" and "var" for now since we don't know whether this is on the left or
    // right side of the equals yet.
    out.result = in.string();
    out.var = in.string();
  }
};

template <>
struct action<filename::FILENAME> {
  template <typename ActionInput>
  static void apply(const ActionInput& in, ActionData& out) {
    std::cout << std::string(out.depth * 2, ' ') << "Found filename: " << in.string() << std::endl;
    out.result = in.string();
  }
};

template <>
struct action<INCLUDE> {
  template <typename ActionInput>
  static void apply(const ActionInput& in, ActionData& out) {
    std::cout << std::string(out.depth * 2, ' ') << "Found include file: " << out.result
              << std::endl;
    try {
      // NOTE: All include files are relative to `EXAMPLE_DIR` for now. This will eventually be an
      // input.
      const auto cfg_file = std::filesystem::path(EXAMPLE_DIR) / out.result;
      peg::file_input include_file(cfg_file);
      std::cout << "nested parse: " << include_file.source() << std::endl;
      peg::parse_nested<config::grammar, config::action>(in.position(), include_file, out);
    } catch (const std::system_error& e) {
      throw peg::parse_error("Include error", in.position());
    }
  }
};

template <>
struct action<FLAT_KEY> {
  template <typename ActionInput>
  static void apply(const ActionInput& in, ActionData& out) {
    // A FLAT_KEY is made up of a bunch of KEYs, but we may not want to consume all keys here
    // (because this could be inside another struct. Nominally, a FLAT_KEY shouldn't be in a
    // struct/proto/reference, but a FLAT_KEY can be used to represent a variable reference, so
    // it might be.
    auto keys = utils::split(in.string(), '.');

    // Ensure that 'out.keys' has enough keys!
    if (out.keys.size() < keys.size()) {
      std::cerr << "[FLAT_KEY] Not enough keys in list!" << std::endl;
      return;
    }
    const auto N_KEYS = keys.size();
    // Walk through the keys, and pop them off of out.keys
    for (size_t i = 0; i < N_KEYS; ++i) {
      // Consume 1 key for every key in "keys"
      // TODO: Check if the keys are the same?
#if VERBOSE_DEBUG
      std::cout << std::string(out.depth * 2, ' ') << "Popping: " << keys.back() << " | "
                << out.keys.back() << std::endl;
#endif
      keys.pop_back();
      out.keys.pop_back();
    }
    out.flat_keys.emplace_back(in.string());
  }
};

template <>
struct action<VAR_REF> {
  template <typename ActionInput>
  static void apply(const ActionInput& in, ActionData& out) {
    // This is a bit hacky. We need to look at what was parsed, and pull the correct keys out of
    // the existing key list.
    out.result = in.string();
    const auto var_ref = utils::trim(in.string(), "$()");
    std::cout << std::string(out.depth * 2, ' ') << "[VAR_REF] Result: " << var_ref << std::endl;
    // Consume the most recent FLAT_KEY.
    std::cout << std::string(out.depth * 2, ' ') << "[VAR_REF] Consuming: '" << out.flat_keys.back()
              << "'" << std::endl;
    out.flat_keys.pop_back();
  }
};

template <>
struct action<PAIR> {
  static void apply0(ActionData& out) {
    // std::cout << "Found pair" << std::endl;
    if (out.keys.empty()) {
      std::cerr << "  !!! Something went wrong !!!" << std::endl;
      return;
    }
    // TODO: Decide if we should put this check here, or at the end. Eventually, we'll combine all
    // of these maps, but for now, we need separate ones in case multiple struct-like's exist with
    // different contents. If we do this here, then we'll have a separate map for each top-level
    // key.
    if (out.objects.empty()) {
      out.cfg_res.push_back({});
    }
    // std::cout << out.keys.back() << " = " << out.result << std::endl;
    auto& data = out.objects.empty() ? out.cfg_res.back() : out.objects.back()->data;
    data[out.keys.back()] = std::move(out.obj_res);

    out.pairs.push_back({out.keys.back(), out.result});
    out.keys.pop_back();
    out.result = DEFAULT_RES;
  }
};

template <>
struct action<FULLPAIR> {
  static void apply0(ActionData& out) {
    // std::cout << "Found Full pair" << std::endl;
    if (out.flat_keys.empty()) {
      std::cerr << "  !!! Something went wrong !!!" << std::endl;
      return;
    }
    // std::cout << out.flat_keys.back() << " = " << out.result << std::endl;

    // TODO: Decide what to do about flat keys? Handle specially or store in normal K/V object?
    out.pairs.push_back({out.flat_keys.back(), out.result});
    out.flat_keys.pop_back();
    out.result = DEFAULT_RES;
  }
};

template <>
struct action<REF_VARADD> {
  static void apply0(ActionData& out) {
    out.special_pairs.push_back({"+" + out.keys.back(), out.result});

    // If we're here, then there must be an object and it must be a reference!
    if (dynamic_pointer_cast<types::ConfigReference>(out.objects.back()) == nullptr) {
      std::cerr << "Error while processing '+" << out.keys.back() << " = " << out.result << "'."
                << std::endl;
      std::cerr << "Struct-like: '" << out.objects.back()->name << "' is not a 'reference'."
                << std::endl;
      throw std::bad_cast();
    }

    out.objects.back()->data[out.keys.back()] = std::move(out.obj_res);

    out.keys.pop_back();
    out.result = DEFAULT_RES;
  }
};

template <>
struct action<REF_VARSUB> {
  static void apply0(ActionData& out) {
    out.special_pairs.push_back({out.var, out.result});
    out.var = DEFAULT_RES;
    out.result = DEFAULT_RES;
  }
};

template <>
struct action<STRUCTs> {
  static void apply0(ActionData& out) {
    out.type_outer = out.type;
    out.type = GroupType::kStruct;
    out.depth++;
    std::cout << std::string(out.depth * 2, ' ') << "Depth is now " << out.depth << std::endl;
    std::cout << std::string(out.depth * 2, ' ') << "struct " << out.keys.back() << std::endl;
    out.objects.push_back(std::make_shared<types::ConfigStruct>(out.keys.back()));
    std::cout << std::string(out.depth * 2, ' ') << "length of objects is: " << out.objects.size()
              << std::endl;
  }
};

template <>
struct action<PROTOs> {
  static void apply0(ActionData& out) {
    out.type_outer = out.type;
    out.type = GroupType::kProto;
    out.depth++;
    std::cout << std::string(out.depth * 2, ' ') << "Depth is now " << out.depth << std::endl;
    std::cout << std::string(out.depth * 2, ' ') << "proto " << out.keys.back() << std::endl;
    out.objects.push_back(std::make_shared<types::ConfigProto>(out.keys.back()));
    std::cout << std::string(out.depth * 2, ' ') << "length of objects is: " << out.objects.size()
              << std::endl;
  }
};

template <>
struct action<REFs> {
  static void apply0(ActionData& out) {
    out.type_outer = out.type;
    out.type = GroupType::kReference;
    out.depth++;
    std::cout << std::string(out.depth * 2, ' ') << "Depth is now " << out.depth << std::endl;
    std::cout << std::string(out.depth * 2, ' ') << "reference " << out.flat_keys.back() << " as "
              << out.keys.back() << std::endl;
    out.objects.push_back(
        std::make_shared<types::ConfigReference>(out.keys.back(), out.flat_keys.back()));
    std::cout << std::string(out.depth * 2, ' ') << "length of objects is: " << out.objects.size()
              << std::endl;
  }
};

template <>
struct action<STRUCT> {
  static void apply0(ActionData& out) {
    // std::cout << "Found Struct: " << out.keys.back() << std::endl;
    // TODO: This isn't really what we want to do, but it's simple enough for now.
    out.struct_names.emplace_back(out.keys.back());

    const auto this_obj = std::move(out.objects.back());
    out.objects.pop_back();
    auto& data = out.objects.empty() ? out.cfg_res.back() : out.objects.back()->data;
    data[out.keys.back()] = std::move(this_obj);

    std::cout << std::string(out.depth * 2, ' ') << "length of objects is: " << out.objects.size()
              << std::endl;
    out.keys.pop_back();
  }
};

template <>
struct action<PROTO> {
  static void apply0(ActionData& out) {
    // std::cout << "Found Proto: " << out.keys.back() << std::endl;
    // TODO: This isn't really what we want to do, but it's simple enough for now.
    out.proto_names.emplace_back(out.keys.back());

    const auto this_obj = std::move(out.objects.back());
    out.objects.pop_back();
    auto& data = out.objects.empty() ? out.cfg_res.back() : out.objects.back()->data;
    data[out.keys.back()] = std::move(this_obj);

    out.keys.pop_back();
  }
};

template <>
struct action<REFERENCE> {
  static void apply0(ActionData& out) {
    // std::cout << "Found Reference: " << out.keys.back() << std::endl;
    // TODO: This isn't really what we want to do, but it's simple enough for now.

    // First, we need to consume the proto that is being referenced.
    std::string ref_info = out.keys.back() + " : " + out.flat_keys.back();
    out.reference_names.emplace_back(ref_info);

    const auto this_obj = std::move(out.objects.back());
    out.objects.pop_back();
    auto& data = out.objects.empty() ? out.cfg_res.back() : out.objects.back()->data;
    data[out.keys.back()] = std::move(this_obj);

    out.keys.pop_back();
    out.flat_keys.pop_back();
  }
};

template <>
struct action<END> {
  static void apply0(ActionData& out) {
    // TODO: If we've found an end tag, then the last two keys should match. If they don't,
    // we've made a mistake!
    if (out.keys.size() < 2) {
      std::cerr << "[END] This is bad. Probably should throw here." << std::endl;
      return;
    }
    const auto end_key = out.keys.back();
    out.keys.pop_back();
    const auto is_match = end_key == out.keys.back();
    std::cout << std::string(out.depth * 2, ' ') << "End key matches struct key? " << is_match
              << std::endl;
    if (!is_match) {
      std::cout << "~~~~~ Debug ~~~~~" << std::endl;
      std::cout << "End key: " << end_key << std::endl;
      out.print();
      std::cout << "##### Debug #####" << std::endl;
    }
    out.depth--;
    std::cout << std::string(out.depth * 2, ' ') << "Depth is now " << out.depth << std::endl;
    out.type = out.type_outer;
  }
};

/*
template <> struct action<HEX> {
  template <typename ActionInput>
  static void apply(const ActionInput &in, std::string &out) {
    std::cout << "In HEX action: " << in.string() << "\n";
    out += "|H" + in.string();
  }
};
template <> struct action<STRING> {
  template <typename ActionInput>
  static void apply(const ActionInput &in, std::string &out) {
    std::cout << "In STRING action: " << in.string() << "\n";
    out += "|S" + in.string();
  }
};
template <> struct action<NUMBER> {
  template <typename ActionInput>
  static void apply(const ActionInput &in, std::string &out) {
    std::cout << "In NUMBER action: " << in.string() << "\n";
    out += "|N" + in.string();
  }
};
*/
}  // namespace config

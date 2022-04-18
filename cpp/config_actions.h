#pragma once

#include <fmt/format.h>

#include <algorithm>
#include <exception>
#include <filesystem>
#include <iostream>
#include <magic_enum.hpp>
#include <map>
#include <memory>
#include <string>
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>
#include <vector>

#include "config_classes.h"
#include "config_exceptions.h"
#include "config_grammar.h"
#include "config_helpers.h"
#include "utils.h"

#define VERBOSE_DEBUG 0

namespace config {

const std::string DEFAULT_RES{"***"};

// Add action to perform when a `proto` is encountered!
struct ActionData {
  int depth{0};  // Nesting level

  std::string result = DEFAULT_RES;
  std::vector<std::string> keys;
  std::vector<std::string> flat_keys;

  // NOTE: A struct, is just a fancy way of describing a key, that contains an array of key/value
  // pairs (much like a json object). We want to be able to represent both a struct and a simple
  // key/value pair, so the top level can't be a struct, but must be more generically representable.

  // Nominally, we'd have just a `types::ConfigBase` at the very root (which we could still do,
  // maybe), but we need to be able to represent an array of items here in order to support
  // duplicate structs, etc. We'll join these all up later.
  std::vector<types::CfgMap> cfg_res{1};
  std::shared_ptr<types::ConfigBase> obj_res;
  std::vector<std::shared_ptr<types::ConfigStructLike>> objects;

  void print() {
    if (!keys.empty()) {
      std::cout << "Keys: \n";
      for (const auto& key : keys) {
        std::cout << "  " << key << "\n";
      }
    }
    if (!flat_keys.empty()) {
      std::cout << "Flat Keys: \n";
      for (const auto& key : flat_keys) {
        std::cout << "  " << key << "\n";
      }
    }
    std::cout << "result: " << result << std::endl;
    std::cout << "obj_res: " << obj_res << std::endl;
    if (!objects.empty()) {
      std::cout << "objects: " << std::endl;
      for (const auto& obj : objects) {
        std::cout << obj << std::endl;
      }
    }
    std::cout << "==========" << std::endl;
    std::cout << "cfg_res: " << std::endl;
    for (const auto& mp : cfg_res) {
      std::cout << mp << std::endl;
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
    std::cout << std::string(out.depth * 2, ' ') << "In VALUE action: " << in.string() << std::endl;
#endif
    if (out.obj_res == nullptr) {
      // NOTE: This should never happen!
      std::cout << std::string(10, '!') << " Creating default ConfigValue object "
                << std::string(10, '!') << std::endl;
      out.obj_res = std::make_shared<types::ConfigValue>(in.string());
    }
    out.obj_res->line = in.position().line;
    out.obj_res->source = in.position().source;
  }
};

template <>
struct action<HEX> {
  template <typename ActionInput>
  static void apply(const ActionInput& in, ActionData& out) {
    const auto hex = std::stoi(in.string(), nullptr, 16);
#if VERBOSE_DEBUG
    std::cout << std::string(out.depth * 2, ' ') << "In HEX action: " << in.string() << "|" << hex
              << "|0x" << std::hex << hex << "\n";
#endif
    out.obj_res = std::make_shared<types::ConfigValue>(in.string(), types::Type::kNumber, hex);
  }
};

template <>
struct action<STRING> {
  template <typename ActionInput>
  static void apply(const ActionInput& in, ActionData& out) {
#if VERBOSE_DEBUG
    std::cout << std::string(out.depth * 2, ' ') << "In STRING action: " << in.string() << "\n";
#endif
    out.obj_res = std::make_shared<types::ConfigValue>(in.string(), types::Type::kString);
  }
};

template <>
struct action<FLOAT> {
  template <typename ActionInput>
  static void apply(const ActionInput& in, ActionData& out) {
#if VERBOSE_DEBUG
    std::cout << std::string(out.depth * 2, ' ') << "In FLOAT action: " << in.string() << std::endl;
#endif
    std::any any_val = std::stod(in.string());

    out.obj_res = std::make_shared<types::ConfigValue>(in.string(), types::Type::kNumber, any_val);
  }
};

template <>
struct action<INTEGER> {
  template <typename ActionInput>
  static void apply(const ActionInput& in, ActionData& out) {
#if VERBOSE_DEBUG
    std::cout << std::string(out.depth * 2, ' ') << "In INTEGER action: " << in.string()
              << std::endl;
#endif
    std::any any_val = std::stoi(in.string());

    out.obj_res = std::make_shared<types::ConfigValue>(in.string(), types::Type::kNumber, any_val);
  }
};

template <>
struct action<LIST> {
  template <typename ActionInput>
  static void apply(const ActionInput& in, ActionData& out) {
    // Split the list into elements
    auto entries = utils::split(utils::trim(in.string(), "[] \t\n\r"), ',');
    // Remove any leading/trailing whitespace from each list element.
    std::transform(entries.begin(), entries.end(), entries.begin(),
                   [](auto s) { return utils::trim(s); });
#if VERBOSE_DEBUG
    std::cout << std::string(out.depth * 2, ' ') << "In LIST action: " << in.string() << "\n";
    std::cout << std::string(out.depth * 2, ' ') << " --- "
              << "Has " << entries.size() << " elements: " << utils::join(entries, "; ") << "\n";
#endif
    // TODO: Create a custom type for lists.
    out.obj_res = std::make_shared<types::ConfigValue>(in.string(), types::Type::kList, entries);
  }
};

template <>
struct action<VAR> {
  template <typename ActionInput>
  static void apply(const ActionInput& in, ActionData& out) {
#if VERBOSE_DEBUG
    std::cout << std::string(out.depth * 2, ' ') << "Found var: " << in.string() << std::endl;
#endif
    // Store string here in `result` because for the case of a `REF_VARSUB` element, storing the
    // result only in the `out.obj_res` will result it in being over-written by the `VALUE` element
    // that is captured.
    out.result = in.string();

    out.obj_res = std::make_shared<types::ConfigVar>(in.string());
    out.obj_res->line = in.position().line;
    out.obj_res->source = in.position().source;
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
    const auto var_ref = utils::trim(in.string(), "$()");
#if VERBOSE_DEBUG
    std::cout << std::string(out.depth * 2, ' ') << "[VAR_REF] Result: " << var_ref << std::endl;
    std::cout << std::string(out.depth * 2, ' ') << "[VAR_REF] Consuming: '" << out.flat_keys.back()
              << "'" << std::endl;
#endif
    out.obj_res = std::make_shared<types::ConfigValueLookup>(var_ref);
    out.obj_res->source = in.position().source;
    out.obj_res->line = in.position().line;

    out.flat_keys.pop_back();
  }
};

template <>
struct action<PAIR> {
  static void apply0(ActionData& out) {
    if (out.keys.empty()) {
      out.print();
      throw InvalidStateException("While processing 'PAIR', no available keys.");
    }

    auto& data = out.objects.empty() ? out.cfg_res.back() : out.objects.back()->data;
    if (data.contains(out.keys.back())) {
      const auto location =
          out.objects.empty() ? std::string("top_level") : out.objects.back()->name;
      throw DuplicateKeyException(
          fmt::format("[PAIR] Duplicate key '{}' found in {}!", out.keys.back(), location));
    }
    data[out.keys.back()] = std::move(out.obj_res);

    out.keys.pop_back();
  }
};

template <>
struct action<FULLPAIR> {
  static void apply0(ActionData& out) {
    if (out.flat_keys.empty()) {
      out.print();
      throw InvalidStateException("[FULLPAIR] Expected to find 'FLAT_KEY', but list is empty.");
    }

    if (!out.objects.empty()) {
      std::cerr << " Found a `FULLPAIR` but expected `objects` list to be empty." << std::endl;
      out.print();
      throw InvalidStateException(fmt::format(
          "[FULLPAIR] Objects list contains {} items. Expected to be empty.", out.objects.size()));
    }

    // TODO: Decide what to do about flat keys? Handle specially or store in normal K/V object?
#if 1
    auto& data = out.cfg_res.back();
    if (data.contains(out.flat_keys.back())) {
      throw DuplicateKeyException(fmt::format("Duplicate key '{}' found!", out.flat_keys.back()));
    }
    data[out.flat_keys.back()] = std::move(out.obj_res);
#else
    // unflatten
    auto keys = utils::split(out.flat_keys.back(), '.');
    const auto c_map = config::helpers::unflatten(std::span{keys}.subspan(0, keys.size() - 1),
                                                  {{keys.back(), std::move(out.obj_res)}});
    // std::cout << c_map << std::endl;
    out.cfg_res.emplace_back(c_map);
#endif
    out.flat_keys.pop_back();
  }
};

template <>
struct action<PROTO_PAIR> {
  static void apply0(ActionData& out) {
#if VERBOSE_DEBUG
    std::cout << std::string(out.depth * 2, ' ') << "[PROTO_VAR] Result: " << out.keys.back()
              << " = " << out.result << std::endl;
    std::cout << std::string(out.depth * 2, ' ') << "[PROTO_VAR] Key count: " << out.keys.size()
              << std::endl;
#endif

    // If we're here, then there must be an object and it must be a proto!
    if (out.objects.back()->type != types::Type::kProto) {
      throw InvalidTypeException(
          fmt::format("Error while processing '{} = {}' in {}. Expected 'proto', found '{}'.",
                      out.keys.back(), out.result, out.objects.back()->name,
                      magic_enum::enum_name<types::Type>(out.objects.back()->type)));
      ;
    }

    // TODO: Check for duplicate keys here!
    if (out.obj_res->type == types::Type::kVar) {
      auto proto = dynamic_pointer_cast<types::ConfigProto>(out.objects.back());
      auto proto_var = std::move(dynamic_pointer_cast<types::ConfigVar>(out.obj_res));
      proto->data[out.keys.back()] = proto_var;
    } else {
      out.objects.back()->data[out.keys.back()] = std::move(out.obj_res);
    }

    out.keys.pop_back();
    out.result = DEFAULT_RES;
#if VERBOSE_DEBUG
    std::cout << "~~~~~ Debug ~~~~~" << std::endl;
    out.print();
    std::cout << "##### Debug #####" << std::endl;
#endif
  }
};

template <>
struct action<REF_VARADD> {
  static void apply0(ActionData& out) {
    // If we're here, then there must be an object and it must be a reference!
    if (out.objects.back()->type != types::Type::kReference) {
      throw InvalidTypeException(
          fmt::format("Error while processing '+{} = {}' in {}. Expected 'reference', found '{}'.",
                      out.keys.back(), out.obj_res, out.objects.back()->name,
                      magic_enum::enum_name<types::Type>(out.objects.back()->type)));
    }

    if (out.objects.back()->data.contains(out.keys.back())) {
      throw DuplicateKeyException(fmt::format(
          "Duplicate key '{}' found in {} ({})!", out.keys.back(), out.objects.back()->name,
          magic_enum::enum_name<types::Type>(out.objects.back()->type)));
    }
    out.objects.back()->data[out.keys.back()] = std::move(out.obj_res);

    out.keys.pop_back();
  }
};

template <>
struct action<REF_VARSUB> {
  static void apply0(ActionData& out) {
    // If we're here, then there must be an object and it must be a reference!
    if (out.objects.back()->type != types::Type::kReference) {
      throw InvalidTypeException(
          fmt::format("Error while processing '{} = {}' in {}. Expected 'reference', found '{}'.",
                      out.result, out.obj_res, out.objects.back()->name,
                      magic_enum::enum_name<types::Type>(out.objects.back()->type)));
    }

    auto ref = dynamic_pointer_cast<types::ConfigReference>(out.objects.back());
    ref->ref_vars[out.result] = std::move(out.obj_res);

    out.result = DEFAULT_RES;
  }
};

template <>
struct action<STRUCTs> {
  static void apply0(ActionData& out) {
    std::cout << std::string(out.depth * 2, ' ') << "struct " << out.keys.back() << std::endl;
    out.objects.push_back(std::make_shared<types::ConfigStruct>(out.keys.back(), out.depth++));
    std::cout << std::string(out.depth * 2, ' ') << "Depth is now " << out.depth << std::endl;
    std::cout << std::string(out.depth * 2, ' ') << "length of objects is: " << out.objects.size()
              << std::endl;
  }
};

template <>
struct action<PROTOs> {
  static void apply0(ActionData& out) {
    std::cout << std::string(out.depth * 2, ' ') << "proto " << out.keys.back() << std::endl;
    out.objects.push_back(std::make_shared<types::ConfigProto>(out.keys.back(), out.depth++));
    std::cout << std::string(out.depth * 2, ' ') << "Depth is now " << out.depth << std::endl;
    std::cout << std::string(out.depth * 2, ' ') << "length of objects is: " << out.objects.size()
              << std::endl;
  }
};

template <>
struct action<REFs> {
  static void apply0(ActionData& out) {
    std::cout << std::string(out.depth * 2, ' ') << "reference " << out.flat_keys.back() << " as "
              << out.keys.back() << std::endl;
    out.objects.push_back(std::make_shared<types::ConfigReference>(
        out.keys.back(), out.flat_keys.back(), out.depth++));
    std::cout << std::string(out.depth * 2, ' ') << "Depth is now " << out.depth << std::endl;
    std::cout << std::string(out.depth * 2, ' ') << "length of objects is: " << out.objects.size()
              << std::endl;
  }
};

template <>
struct action<STRUCT> {
  static void apply0(ActionData& out) {
    const auto this_obj = std::move(out.objects.back());
    out.objects.pop_back();
    auto& data = out.objects.empty() ? out.cfg_res.back() : out.objects.back()->data;
    if (data.contains(out.keys.back())) {
      const auto location =
          out.objects.empty()
              ? std::string("top_level")
              : fmt::format("{} ({})", out.objects.back()->name,
                            magic_enum::enum_name<types::Type>(out.objects.back()->type));
      throw DuplicateKeyException(
          fmt::format("Duplicate key '{}' found in {}!", out.keys.back(), location));
    }
    data[out.keys.back()] = std::move(this_obj);

    // NOTE: Nothing else left in the objects buffer? Create a new element in the `cfg_res` vector
    // in case we have a duplicat struct later. We won't resolve that now, but later in another
    // pass.
    if (out.objects.empty()) {
      out.cfg_res.push_back({});
    }

    std::cout << std::string(out.depth * 2, ' ') << "length of objects is: " << out.objects.size()
              << std::endl;
    out.keys.pop_back();
  }
};

template <>
struct action<PROTO> {
  static void apply0(ActionData& out) {
    const auto this_obj = std::move(out.objects.back());
    out.objects.pop_back();
    auto& data = out.objects.empty() ? out.cfg_res.back() : out.objects.back()->data;
    if (data.contains(out.keys.back())) {
      const auto location =
          out.objects.empty()
              ? std::string("top_level")
              : fmt::format("{} ({})", out.objects.back()->name,
                            magic_enum::enum_name<types::Type>(out.objects.back()->type));
      throw DuplicateKeyException(
          fmt::format("Duplicate key '{}' found in {}!", out.keys.back(), location));
    }
    data[out.keys.back()] = std::move(this_obj);

    // NOTE: Nothing else left in the objects buffer? Create a new element in the `cfg_res` vector
    // in case we have a duplicat struct later. We won't resolve that now, but later in another
    // pass.
    if (out.objects.empty()) {
      out.cfg_res.push_back({});
    }

    out.keys.pop_back();
  }
};

template <>
struct action<REFERENCE> {
  static void apply0(ActionData& out) {
    const auto this_obj = std::move(out.objects.back());
    out.objects.pop_back();
    auto& data = out.objects.empty() ? out.cfg_res.back() : out.objects.back()->data;
    if (data.contains(out.keys.back())) {
      const auto location =
          out.objects.empty()
              ? std::string("top_level")
              : fmt::format("{} ({})", out.objects.back()->name,
                            magic_enum::enum_name<types::Type>(out.objects.back()->type));
      throw DuplicateKeyException(
          fmt::format("Duplicate key '{}' found in {}!", out.keys.back(), location));
    }
    data[out.keys.back()] = std::move(this_obj);

    // NOTE: Nothing else left in the objects buffer? Create a new element in the `cfg_res` vector
    // in case we have a duplicat struct later. We won't resolve that now, but later in another
    // pass.
    if (out.objects.empty()) {
      out.cfg_res.push_back({});
    }

    out.keys.pop_back();
    out.flat_keys.pop_back();
  }
};

template <>
struct action<END> {
  static void apply0(ActionData& out) {
    if (out.keys.size() < 2) {
      out.print();
      throw InvalidStateException(
          fmt::format("[END] Expected 2 or more keys, found {}.", out.keys.size()));
    }
    const auto end_key = out.keys.back();
    out.keys.pop_back();
    // If we've found an end tag, then the last two keys should match. If they don't, the config is
    // malformed.
    const auto is_match = end_key == out.keys.back();
    if (!is_match) {
      out.print();
      throw InvalidConfigException(
          fmt::format("End key mismatch: {} != {}.", end_key, out.keys.back()));
    }
    out.depth--;
    std::cout << std::string(out.depth * 2, ' ') << "Depth is now " << out.depth << std::endl;
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

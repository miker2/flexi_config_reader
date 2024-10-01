#pragma once

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <range/v3/view/map.hpp>
#include <span>
#include <string>
#include <string_view>
#include <tao/pegtl.hpp>
#include <unordered_set>
#include <utility>
#include <vector>

#include "flexi_cfg/config/classes.h"
#include "flexi_cfg/config/exceptions.h"
#include "flexi_cfg/config/grammar.h"
#include "flexi_cfg/config/helpers.h"
#include "flexi_cfg/config/parser-internal.h"
#include "flexi_cfg/logger.h"
#include "flexi_cfg/utils.h"

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CONFIG_ACTION_DEBUG(MSG_F, ...) \
  logger::debug("{}" MSG_F, std::string(out.depth * 2UL, ' '), ##__VA_ARGS__);

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CONFIG_ACTION_TRACE(MSG_F, ...) \
  logger::trace("{}" MSG_F, std::string(out.depth * 2UL, ' '), ##__VA_ARGS__);

namespace flexi_cfg::config {

constexpr std::string_view DEFAULT_RES{"***"};

struct path_hash {
  std::size_t operator()(std::filesystem::path const& p) const {
    return std::hash<std::string>()(p.string());
  }
};

// Holds state as we make progress parsing the config document(s)
struct ActionData {
  // include file metadata as it's processing
  struct IncludeData {
    std::string file;
    // [optional]
    bool is_optional{false};
    // [once]
    bool is_once{false};
    bool is_relative{false};
  };

  ActionData() : ActionData(std::filesystem::current_path()) {}
  explicit ActionData(const std::filesystem::path& base) : base_dir(base) {}

  int depth{0};  // Nesting level

  std::filesystem::path base_dir;
  std::optional<IncludeData> include_pending;
  std::unordered_set<std::filesystem::path, path_hash> all_files{};  // catch duplicate includes
  std::string result{DEFAULT_RES};
  std::vector<std::string> keys;
  std::vector<std::string> flat_keys;
  bool in_proto{false};
  std::string proto_key{};

#if 0
  // I'd prefer something like this that provides a bit more type safety, but this map can't be
  // implicitly convert to a CfgMap object.
  std::map<std::string, std::shared_ptr<types::ConfigValueLookup>>
      value_lookups;  // This is purely for keeping track of any value lookup objects that might be
                      // contained within an expression object.
#else
  types::CfgMap value_lookups;
#endif

  // NOTE: A struct, is just a fancy way of describing a key, that contains an array of key/value
  // pairs (much like a json object). We want to be able to represent both a struct and a simple
  // key/value pair, so the top level can't be a struct, but must be more generically representable.

  // Nominally, we'd have just a `types::ConfigBase` at the very root (which we could still do,
  // maybe), but we need to be able to represent an array of items here in order to support
  // duplicate structs, etc. We'll join these all up later.
  std::vector<types::CfgMap> cfg_res{1};
  std::shared_ptr<types::ConfigBase> obj_res;
  std::vector<std::shared_ptr<types::ConfigList>> lists;
  std::vector<std::shared_ptr<types::ConfigStructLike>> objects;

  // Special data for overrides. We need to track these separately to make it easier to confirm
  // things are being overwritten
  bool is_override{false};
  types::CfgMap override_values;  // A set of FLAT_KEY / VALUE pairs (to be resolved later)

  void print(std::ostream& os) const {
    if (in_proto) {
      os << "current proto key: " << proto_key << "\n";
    }
    if (!keys.empty()) {
      os << "Keys: \n";
      for (const auto& key : keys) {
        os << "  " << key << "\n";
      }
    }
    if (!flat_keys.empty()) {
      os << "Flat Keys: \n";
      for (const auto& key : flat_keys) {
        os << "  " << key << "\n";
      }
    }
    os << "result: " << result << std::endl;
    os << "obj_res: " << obj_res << std::endl;
    if (!objects.empty()) {
      os << "objects: " << std::endl;
      for (const auto& obj : objects) {
        os << obj << std::endl;
      }
    }
    if (!value_lookups.empty()) {
      os << "value_lookups: \n";
      for (const auto& s : value_lookups) {
        os << "  " << s.first << "\n";
      }
    }
    os << "==========" << std::endl;
    os << "cfg_res: " << std::endl;
    for (const auto& mp : cfg_res) {
      os << mp << std::endl;
    }
    if (!lists.empty()) {
      os << "lists: \n";
      for (const auto& l : lists) {
        os << l << std::endl;
      }
    }

    os << "is_override: " << (is_override ? "true" : "false") << std::endl;
    if (!override_values.empty()) {
      os << "override_values: \n" << override_values << std::endl;
    }

    os << "^^^^^^^^^^" << std::endl;
  }
};

template <typename Rule>
struct action : peg::nothing<Rule> {};

template <>
struct action<KEY> {
  template <typename ActionInput>
  static void apply(const ActionInput& in, ActionData& out) {
#if VERBOSE_DEBUG_ACTIONS
    CONFIG_ACTION_TRACE("In KEY action: '{}'", in.string());
#endif

    out.keys.emplace_back(in.string());
    out.is_override = false;  // Reset the override flag
  }
};

template <>
struct action<OVERRIDEk> {
  template <typename ActionInput>
  static void apply(const ActionInput& in, ActionData& out) {
#if VERBOSE_DEBUG_ACTIONS
    CONFIG_ACTION_TRACE("In OVERRIDEk action: '{}'", in.string());
#endif
    out.is_override = true;
  }
};

template <>
struct action<VALUE> {
  template <typename ActionInput>
  static void apply(const ActionInput& in, ActionData& out) {
    if (out.obj_res == nullptr) {
      // NOTE: This should never happen! This probably means that an action corresponding to a token
      // is missing.
      const auto pre_post = std::string(10, '!');
      logger::error("{0} Action for token (corresponding to {} - {}:{}) appears to be missing {0}",
                    pre_post, in.string(), in.position().source, in.position().line);
      throw std::logic_error(
          "The 'VALUE' action should never be executed on a nullptr! This is likely the result of "
          "a new token being added to the grammar without a corresponding action being created.");
    }
    if (out.obj_res != nullptr && out.obj_res->type == types::Type::kValue) {
      // NOTE: The `kValue` type exists only for testing purposes. An object should never be created
      // of this type when parsing a file. This is likely the result of an ill-formed action.
      const auto pre_post = std::string(10, '!');
      logger::error(
          "{0} Action for token at {}:{} resulted in {} type. This is the result of a "
          "misconfigured action that is a child of the `VALUE` token. {0}",
          pre_post, in.position().source, in.position().line, out.obj_res->type);
      throw std::logic_error(
          fmt::format("The 'VALUE' action should never be called on an object of type '{}'. This "
                      "is the result of a misconfigured action.",
                      config::types::Type::kValue));
    }
#if VERBOSE_DEBUG_ACTIONS
    CONFIG_ACTION_TRACE("In VALUE ({}) action: {}", out.obj_res->type, in.string());
#endif
    out.obj_res->line = in.position().line;
    out.obj_res->source = in.position().source;
  }
};

template <>
struct action<HEX> {
  template <typename ActionInput>
  static void apply(const ActionInput& in, ActionData& out) {
    const auto hex = std::stoull(in.string(), nullptr, 16);

#if VERBOSE_DEBUG_ACTIONS
    CONFIG_ACTION_TRACE("In HEX action: {}|{}|0x{:X}", in.string(), hex, hex);
#endif
    out.obj_res = std::make_shared<types::ConfigValue>(in.string(), types::Type::kNumber, hex);
  }
};

template <>
struct action<STRING> {
  template <typename ActionInput>
  static void apply(const ActionInput& in, ActionData& out) {
#if VERBOSE_DEBUG_ACTIONS
    CONFIG_ACTION_TRACE("In STRING action: {}", in.string());
#endif
    out.obj_res = std::make_shared<types::ConfigValue>(in.string(), types::Type::kString);
  }
};

template <>
struct action<FLOAT> {
  template <typename ActionInput>
  static void apply(const ActionInput& in, ActionData& out) {
#if VERBOSE_DEBUG_ACTIONS
    CONFIG_ACTION_TRACE("In FLOAT action: {}", in.string());
#endif
    std::any any_val = std::stod(in.string());

    out.obj_res = std::make_shared<types::ConfigValue>(in.string(), types::Type::kNumber, any_val);
  }
};

template <>
struct action<INTEGER> {
  template <typename ActionInput>
  static void apply(const ActionInput& in, ActionData& out) {
#if VERBOSE_DEBUG_ACTIONS
    CONFIG_ACTION_TRACE("In INTEGER action: {}", in.string());
#endif
    std::any any_val = std::stoi(in.string());

    out.obj_res = std::make_shared<types::ConfigValue>(in.string(), types::Type::kNumber, any_val);
  }
};

// This action isn't necessary as it does nothing, but it is useful for debugging the parsing of a
// config file.
template <>
struct action<BOOLEAN> {
  static void apply0(ActionData& out) {
#if VERBOSE_DEBUG_ACTIONS
    CONFIG_ACTION_TRACE("In BOOLEAN action...");
#endif
  }
};

template <>
struct action<TRUE> {
  template <typename ActionInput>
  static void apply(const ActionInput& in, ActionData& out) {
#if VERBOSE_DEBUG_ACTIONS
    CONFIG_ACTION_TRACE("In TRUE action: {}", in.string());
#endif
    std::any any_val = true;

    out.obj_res = std::make_shared<types::ConfigValue>(in.string(), types::Type::kBoolean, any_val);
  }
};

template <>
struct action<FALSE> {
  template <typename ActionInput>
  static void apply(const ActionInput& in, ActionData& out) {
#if VERBOSE_DEBUG_ACTIONS
    CONFIG_ACTION_TRACE("In FALSE action: {}", in.string());
#endif
    std::any any_val = false;

    out.obj_res = std::make_shared<types::ConfigValue>(in.string(), types::Type::kBoolean, any_val);
  }
};

template <>
/* struct action<PROTO_LIST::begin> */
struct action<LIST::begin> {
  static void apply0(ActionData& out) {
    CONFIG_ACTION_TRACE("In LIST::begin action - creating {}", types::Type::kList);
    out.lists.push_back(std::make_shared<types::ConfigList>());
  }
};

template <>
struct action<LIST::element> {
  static void apply0(ActionData& out) {
    CONFIG_ACTION_TRACE("In LIST::element action - adding {}", out.obj_res);
    // Add (or check) the type of the elements in the list
    if (!helpers::listElementValid(out.lists.back(), out.obj_res->type)) {
      const auto key = !out.keys.empty() ? out.keys.back() : "";
      THROW_EXCEPTION(InvalidTypeException,
                      "While processing '{}' at {}, found {}, but expected {}. All elements in {} "
                      "must be of the same type.",
                      key, out.obj_res->loc(), out.obj_res->type,
                      out.lists.back()->list_element_type, out.lists.back()->type);
    }
    out.lists.back()->data.push_back(std::move(out.obj_res));
    out.obj_res = nullptr;
  }
};

template <>
/* struct action<PROTO_LIST::end> */
struct action<LIST::end> {
  static void apply0(ActionData& out) {
    CONFIG_ACTION_TRACE("In LIST::end action - completing LIST");
    out.obj_res = std::move(out.lists.back());
    out.lists.pop_back();
  }
};

template <>
struct action<PROTO_LIST::element> {
  static void apply0(ActionData& out) {
    CONFIG_ACTION_TRACE("In PROTO_LIST::element action - adding {}", out.obj_res);
    // Add (or check) the type of the elements in the list
    if (!helpers::listElementValid(out.lists.back(), out.obj_res->type)) {
      const auto key = !out.keys.empty() ? out.keys.back() : "";
      THROW_EXCEPTION(InvalidTypeException,
                      "While processing '{}' at {}, found {}, but expected {}. All elements in {} "
                      "must be of the same type.",
                      key, out.obj_res->loc(), out.obj_res->type,
                      out.lists.back()->list_element_type, out.lists.back()->type);
    }
    out.lists.back()->data.push_back(std::move(out.obj_res));
    out.obj_res = nullptr;
  }
};

template <>
struct action<Eo> {
  static void apply0(ActionData& out) {
    // Reset the value lookup map since we're starting a new expression.
    out.value_lookups.clear();
  }
};

template <>
struct action<EXPRESSION> {
  template <typename ActionInput>
  static void apply(const ActionInput& in, ActionData& out) {
#if VERBOSE_DEBUG_ACTIONS
    CONFIG_ACTION_TRACE("In EXPRESSION action: {}", in.string());
#endif
    CONFIG_ACTION_TRACE("EXPRESSION contains the following VALUE_LOOKUP objects: {}",
                        ranges::views::values(out.value_lookups));
    // Grab the entire input and stuff it into a ConfigExpression. We'll properly evaluate it later.

    out.obj_res = std::make_shared<types::ConfigExpression>(in.string(), out.value_lookups);
    out.value_lookups.clear();
    out.obj_res->line = in.position().line;
    out.obj_res->source = in.position().source;
  }
};

template <>
struct action<VAR> {
  template <typename ActionInput>
  static void apply(const ActionInput& in, ActionData& out) {
#if VERBOSE_DEBUG_ACTIONS
    CONFIG_ACTION_TRACE("Found var: {}", in.string());
#endif
    // Store string here in `result` because for the case of a `REF_VARDEF` element, storing the
    // result only in the `out.obj_res` will result it in being over-written by the `VALUE` element
    // that is captured.
    out.result = in.string();

    out.obj_res = std::make_shared<types::ConfigVar>(in.string());
    out.obj_res->line = in.position().line;
    out.obj_res->source = in.position().source;
  }
};

template <>
struct action<PARENTNAMEk> {
  static void apply0(ActionData& out) {
    // When this action is invoked, we create a ConfigValue (of type kString) whose value matches
    // the name/key of the parent object. This allows us to easily reference the name of the parent
    // object where required.
    out.obj_res =
        std::make_shared<types::ConfigValue>(out.objects.back()->name, types::Type::kString);
  }
};

template <>
struct action<filename::FILENAME> {
  template <typename ActionInput>
  static void apply(const ActionInput& in, ActionData& out) {
    CONFIG_ACTION_DEBUG("Found filename: {}", in.string());
    assert(out.include_pending.has_value());
    out.include_pending->file = in.string();
  }
};

template <>
struct action<INCLUDEk> {
  template <typename ActionInput>
  static void apply(const ActionInput& in, ActionData& out) {
    out.include_pending = ActionData::IncludeData{.is_relative = false};
    CONFIG_ACTION_DEBUG("Found INCLUDE: {}", in.string());
  }
};

template <>
struct action<INCLUDE_RELATIVEk> {
  template <typename ActionInput>
  static void apply(const ActionInput& in, ActionData& out) {
    out.include_pending = ActionData::IncludeData{.is_relative = true};
    CONFIG_ACTION_DEBUG("Found INCLUDE_RELATIVE: {}", in.string());
  }
};

template <>
struct action<OPTIONALk> {
  template <typename ActionInput>
  static void apply(const ActionInput& in, ActionData& out) {
    CONFIG_ACTION_DEBUG("Found OPTIONAL: {}", in.string());
    assert(out.include_pending.has_value());
    out.include_pending->is_optional = true;
  }
};

template <>
struct action<ONCEk> {
  template <typename ActionInput>
  static void apply(const ActionInput& in, ActionData& out) {
    CONFIG_ACTION_DEBUG("Found ONCE: {}", in.string());
    assert(out.include_pending.has_value());
    out.include_pending->is_once = true;
  }
};

struct base_include_action {
  template <typename ActionInput>
  static void apply(const ActionInput& in, ActionData& out) {
    assert(out.include_pending.has_value());
    auto incl = std::move(out.include_pending.value());
    CONFIG_ACTION_DEBUG("Found include file: {} - (optional: {}, relative: {}, once: {})",
                        incl.file, incl.is_optional, incl.is_relative, incl.is_once);

    try {
      // Substitute any environment variables in the filename
      auto source = incl.file;
      incl.file = utils::substituteEnvVars(incl.file);
      CONFIG_ACTION_DEBUG("Include file after env var substitution: {}", incl.file);
      CONFIG_ACTION_DEBUG("Basedir: {}", out.base_dir.string());

      const auto is_absolute = std::filesystem::path(incl.file).is_absolute();

      const auto cfg_file =
          absolute(is_absolute ? std::filesystem::path(incl.file) : out.base_dir / incl.file);

      CONFIG_ACTION_DEBUG("Resolved include: {}", cfg_file.string());

      if (!exists(cfg_file)) {
        if (incl.is_optional) {
          logger::warn("Skipping, [optional] include (not found): {} -> {}", source,
                       cfg_file.string());
          return;
        }
        throw peg::parse_error(
            fmt::format("Missing include file, consider using 'include [optional] {} -> {}'",
                        source, cfg_file.string()),
            in.position());
      }

      utils::ScopedOverride base_dir_override{out.base_dir};
      if (incl.is_relative) [[unlikely]] {
        base_dir_override.override(cfg_file.parent_path());
      }
      peg::file_input include_file(cfg_file);

      if (out.all_files.contains(cfg_file)) {
        if (incl.is_once) {
          logger::warn("Skipping [once] include (duplicate): {} -> {}", source, cfg_file.string());
          return;
        } else {
          logger::error("Duplicate include file detected: {}", cfg_file.string());
          throw peg::parse_error(
              fmt::format(
                  "duplicate includes are not allowed, consider using 'include [once] {}' -> {}",
                  source, cfg_file.string()),
              in.position());
        }
      }
      out.all_files.insert(cfg_file);
      logger::debug("Begin nested parse: {}", cfg_file.string());
      internal::parseNestedCore<grammar, action, control>(in.position(), include_file, out);
      logger::debug("End nested parse: {}", cfg_file.string());
    } catch (const std::system_error& e) {
      throw peg::parse_error(fmt::format("Include error: {}", e.what()), in.position());
    }
  }
};

template <>
struct action<INCLUDE> : base_include_action {
  template <typename ActionInput>
  static void apply(const ActionInput& in, ActionData& out) {
    base_include_action::apply(in, out);
  }
};

template <>
struct action<INCLUDE_RELATIVE> : base_include_action {
  template <typename ActionInput>
  static void apply(const ActionInput& in, ActionData& out) {
    base_include_action::apply(in, out);
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
      logger::error("[FLAT_KEY] Not enough keys in list!");
      return;
    }
    const auto N_KEYS = keys.size();
    // Walk through the keys, and pop them off of out.keys
    for (size_t i = 0; i < N_KEYS; ++i) {
      // Consume 1 key for every key in "keys"
      assert(keys.back() == out.keys.back());  // NOLINT
#if VERBOSE_DEBUG_ACTIONS
      CONFIG_ACTION_TRACE("Popping: {} | {}", keys.back(), out.keys.back());
#endif
      keys.pop_back();
      out.keys.pop_back();
    }
    out.flat_keys.emplace_back(in.string());
  }
};

template <>
struct action<VALUE_LOOKUP> {
  template <typename ActionInput>
  static void apply(const ActionInput& in, ActionData& out) {
    // This is a bit hacky. We need to look at what was parsed, and pull the correct keys out of
    // the existing key list.

    // We can't use `utils::trim(in.string(), "$()")` here, because if the contents of the
    // VALUE_LOOKUP starts with a `VAR` then the leading `$` of the `VAR` will also be removed.
    const auto var_ref = utils::trim(utils::removeSubStr(in.string(), "$("), ")");
#if VERBOSE_DEBUG_ACTIONS
    CONFIG_ACTION_TRACE("[VALUE_LOOKUP] Result: {}", var_ref);
#endif
    const auto parts = utils::split(var_ref, '.');
    for (auto it = parts.crbegin(); it != parts.crend(); ++it) {
      if (!out.keys.empty() && out.keys.back() == *it) {
#if VERBOSE_DEBUG_ACTIONS
        CONFIG_ACTION_TRACE("[VALUE_LOOKUP] Consuming: '{}'", out.keys.back());
#endif
        out.keys.pop_back();
      }
    }
    auto val_lookup = std::make_shared<types::ConfigValueLookup>(var_ref);
    out.obj_res = val_lookup;
    out.obj_res->source = in.position().source;
    out.obj_res->line = in.position().line;
    out.value_lookups[var_ref] = val_lookup;
  }
};

template <>
struct action<PAIR> {
  static void apply0(ActionData& out) {
    if (out.keys.empty()) {
      out.print(std::cerr);
      THROW_EXCEPTION(InvalidStateException, "While processing 'PAIR', no available keys.");
    }

    auto& data = out.objects.empty() ? out.cfg_res.back() : out.objects.back()->data;

    if (!out.is_override && data.contains(out.keys.back())) {
      const auto location =
          out.objects.empty() ? std::string("top_level") : out.objects.back()->name;
      THROW_EXCEPTION(DuplicateKeyException,
                      "Duplicate key '{}' found in {}! "
                      "Previously encountered at {} ({}), now at {} ({})",
                      out.keys.back(), location, data[out.keys.back()]->loc(),
                      data[out.keys.back()]->type, out.obj_res->loc(), out.obj_res->type);
    }
    CONFIG_ACTION_TRACE("In PAIR action: '{} = {}'", out.keys.back(), out.obj_res);

    if (out.is_override) {
      auto flat_key = fmt::format("{}", fmt::join(out.keys, "."));
      if (out.override_values.contains(flat_key)) {
        THROW_EXCEPTION(DuplicateOverrideException,
                        "Duplicate key '{}' found in override_values! "
                        "Previously encountered at {} ({}), now at {} ({})",
                        flat_key, out.override_values[flat_key]->loc(),
                        out.override_values[flat_key]->type, out.obj_res->loc(), out.obj_res->type);
      }
      CONFIG_ACTION_TRACE("Adding to override_values: '{} = {}'", flat_key, out.obj_res);
      out.override_values[flat_key] = std::move(out.obj_res);
      out.obj_res = nullptr;
    } else {
      data[out.keys.back()] = std::move(out.obj_res);
      out.obj_res = nullptr;
    }

    out.keys.pop_back();
  }
};

template <>
struct action<FULLPAIR> {
  static void apply0(ActionData& out) {
    if (out.flat_keys.empty()) {
      out.print(std::cerr);
      THROW_EXCEPTION(InvalidStateException,
                      "[FULLPAIR] Expected to find 'FLAT_KEY', but list is empty.");
    }

    if (!out.objects.empty()) {
      logger::error(" Found a `FULLPAIR` but expected `objects` list to be empty.");
      out.print(std::cerr);
      THROW_EXCEPTION(InvalidStateException,
                      "[FULLPAIR] Objects list contains {} items. Expected to be empty.",
                      out.objects.size());
    }
    CONFIG_ACTION_TRACE("In FULLPAIR action: '{} = {}'", out.flat_keys.back(), out.obj_res);

    if (out.is_override) {
      if (out.override_values.contains(out.flat_keys.back())) {
        THROW_EXCEPTION(DuplicateOverrideException,
                        "Duplicate key '{}' found in override_values! "
                        "Previously encountered at {} ({}), now at {} ({})",
                        out.flat_keys.back(), out.override_values[out.flat_keys.back()]->loc(),
                        out.override_values[out.flat_keys.back()]->type, out.obj_res->loc(),
                        out.obj_res->type);
      }
      CONFIG_ACTION_TRACE("Adding to override_values: '{} = {}'", out.flat_keys.back(),
                          out.obj_res);
      out.override_values[out.flat_keys.back()] = std::move(out.obj_res);
      out.obj_res = nullptr;
    } else {
      auto keys = utils::split(out.flat_keys.back(), '.');
      const auto c_map = config::helpers::unflatten(std::span{keys}.subspan(0, keys.size() - 1),
                                                    {{keys.back(), std::move(out.obj_res)}});
      out.obj_res = nullptr;
      out.cfg_res.emplace_back(c_map);
    }
    out.flat_keys.pop_back();
  }
};

template <>
struct action<PROTO_PAIR> {
  static void apply0(ActionData& out) {
#if VERBOSE_DEBUG_ACTIONS
    CONFIG_ACTION_TRACE("[PROTO_VAR] Result: {} = {}", out.keys.back(), out.result);
    CONFIG_ACTION_TRACE("[PROTO_VAR] Key count: {}", out.keys.size());
#endif

    // If we're here, then there must be an object and it must be a proto!
    if (out.objects.back()->type != types::Type::kProto &&
        out.objects.back()->type != types::Type::kStructInProto) {
      // The only way to get here is if there is an error in the grammar!
      THROW_EXCEPTION(InvalidTypeException,
                      "Error while processing '{} = {}' in {}. Expected 'proto', found '{}'.",
                      out.keys.back(), out.result, out.objects.back()->name,
                      out.objects.back()->type);
    }

    if (out.objects.back() && out.objects.back()->data.contains(out.keys.back())) {
      THROW_EXCEPTION(DuplicateKeyException,
                      "Duplicate key '{}' found in {} ({})! "
                      "Previously encountered at {} ({}), now at {} ({})",
                      out.keys.back(), out.objects.back()->name, out.objects.back()->type,
                      out.objects.back()->data[out.keys.back()]->loc(),
                      out.objects.back()->data[out.keys.back()]->type, out.obj_res->loc(),
                      out.obj_res->type);
    }
    if (out.obj_res->type == types::Type::kVar) {
      auto proto = dynamic_pointer_cast<types::ConfigStructLike>(out.objects.back());
      auto proto_var = std::move(dynamic_pointer_cast<types::ConfigVar>(out.obj_res));
      out.obj_res = nullptr;
      proto->data[out.keys.back()] = proto_var;
    } else {
      out.objects.back()->data[out.keys.back()] = std::move(out.obj_res);
      out.obj_res = nullptr;
    }

    out.keys.pop_back();
    out.result = DEFAULT_RES;
  }
};

template <>
struct action<REF_ADDKVP> {
  static void apply0(ActionData& out) {
    // If we're here, then there must be an object and it must be a reference!
    if (out.objects.back()->type != types::Type::kReference) {
      // The only way to get here is if there is an error in the grammar!
      THROW_EXCEPTION(InvalidTypeException,
                      "Error while processing '+{} = {}' in {}. Expected 'reference', found '{}'.",
                      out.keys.back(), out.obj_res, out.objects.back()->name,
                      out.objects.back()->type);
    }

    if (out.objects.back()->data.contains(out.keys.back())) {
      THROW_EXCEPTION(DuplicateKeyException,
                      "Duplicate key '{}' found in {} ({})! "
                      "Previously encountered at {} ({}), now at {} ({})",
                      out.keys.back(), out.objects.back()->name, out.objects.back()->type,
                      out.objects.back()->data[out.keys.back()]->loc(),
                      out.objects.back()->data[out.keys.back()]->type, out.obj_res->loc(),
                      out.obj_res->type);
    }
    CONFIG_ACTION_TRACE("In REF_ADDKVP action: '+{} = {}'", out.keys.back(), out.obj_res);

    out.objects.back()->data[out.keys.back()] = std::move(out.obj_res);
    out.obj_res = nullptr;

    out.keys.pop_back();
  }
};

template <>
struct action<REF_VARDEF> {
  static void apply0(ActionData& out) {
    // If we're here, then there must be an object and it must be a reference!
    if (out.objects.back()->type != types::Type::kReference) {
      // The only way to get here is if there is an error in the grammar!
      THROW_EXCEPTION(InvalidTypeException,
                      "Error while processing '{} = {}' in {}. Expected 'reference', found '{}'.",
                      out.result, out.obj_res, out.objects.back()->name, out.objects.back()->type);
    }
    CONFIG_ACTION_TRACE("In REF_VARDEF action: '{} = {}'", out.result, out.obj_res);

    auto ref = dynamic_pointer_cast<types::ConfigReference>(out.objects.back());
    ref->ref_vars[out.result] = std::move(out.obj_res);
    out.obj_res = nullptr;

    out.result = DEFAULT_RES;
  }
};

template <>
struct action<STRUCTs> {
  static void apply0(ActionData& out) {
    types::Type struct_type = out.in_proto ? types::Type::kStructInProto : types::Type::kStruct;
    CONFIG_ACTION_DEBUG("struct {} - type: {}", out.keys.back(), struct_type);
    out.objects.push_back(
        std::make_shared<types::ConfigStruct>(out.keys.back(), out.depth++, struct_type));
    CONFIG_ACTION_DEBUG("Depth is now {}", out.depth);
    CONFIG_ACTION_DEBUG("length of objects is: {}", out.objects.size());
  }
};

template <>
struct action<PROTOs> {
  static void apply0(ActionData& out) {
    CONFIG_ACTION_DEBUG("proto {}", out.keys.back());
    out.objects.push_back(std::make_shared<types::ConfigProto>(out.keys.back(), out.depth++));
    CONFIG_ACTION_DEBUG("Depth is now {}", out.depth);
    CONFIG_ACTION_DEBUG("length of objects is: {}", out.objects.size());
    out.in_proto = true;
    out.proto_key = utils::join(out.keys, ".");
  }
};

template <>
struct action<REFs> {
  static void apply0(ActionData& out) {
    CONFIG_ACTION_DEBUG("reference {} as {}", out.flat_keys.back(), out.keys.back());
    out.objects.push_back(std::make_shared<types::ConfigReference>(
        out.keys.back(), out.flat_keys.back(), out.depth++));
    CONFIG_ACTION_DEBUG("Depth is now {}", out.depth);
    CONFIG_ACTION_DEBUG("length of objects is: {}", out.objects.size());
  }
};

template <>
struct action<STRUCT> {
  static void apply0(ActionData& out) {
    const auto this_obj = std::move(out.objects.back());
    out.objects.pop_back();
    auto& data = out.objects.empty() ? out.cfg_res.back() : out.objects.back()->data;
    if (data.contains(out.keys.back())) {
      const auto location = out.objects.empty() ? std::string("top_level")
                                                : fmt::format("{}", out.objects.back()->name);
      THROW_EXCEPTION(
          DuplicateKeyException,
          "Duplicate key '{}' found in '{}' - Previously defined at {}, now defined as {}",
          out.keys.back(), location, data[out.keys.back()]->type, types::Type::kStruct);
    }
    data[out.keys.back()] = this_obj;

    // NOTE: Nothing else left in the objects buffer? Create a new element in the `cfg_res` vector
    // in case we have a duplicat struct later. We won't resolve that now, but later in another
    // pass.
    if (out.objects.empty()) {
      out.cfg_res.emplace_back();
    }

    CONFIG_ACTION_DEBUG("length of objects is: {}", out.objects.size());
    out.keys.pop_back();
  }
};

template <>
struct action<STRUCT_IN_PROTO> : action<STRUCT> {};

template <>
struct action<PROTO> {
  static void apply0(ActionData& out) {
    const auto this_obj = std::move(out.objects.back());
    out.objects.pop_back();
    auto& data = out.objects.empty() ? out.cfg_res.back() : out.objects.back()->data;
    if (data.contains(out.keys.back())) {
      const auto location = out.objects.empty() ? std::string("top_level")
                                                : fmt::format("{}", out.objects.back()->name);
      THROW_EXCEPTION(
          DuplicateKeyException,
          "Duplicate key '{}' found in '{}' - Previously defined at {}, now defined as {}",
          out.keys.back(), location, data[out.keys.back()]->type, types::Type::kProto);
    }
    data[out.keys.back()] = this_obj;

    // NOTE: Nothing else left in the objects buffer? Create a new element in the `cfg_res` vector
    // in case we have a duplicate struct later. We won't resolve that now, but later in another
    // pass.
    if (out.objects.empty()) {
      out.cfg_res.emplace_back();
    }
    // Nested protos are not allowed, so it's okay to set this to false always.
    const auto flat_key = utils::join(out.keys, ".");
    if (out.in_proto && out.proto_key == flat_key) {
      out.in_proto = false;
      out.proto_key = {};
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
      const auto location = out.objects.empty() ? std::string("top_level")
                                                : fmt::format("{}", out.objects.back()->name);
      THROW_EXCEPTION(
          DuplicateKeyException,
          "Duplicate key '{}' found in '{}' - Previously defined at {}, now defined as {}",
          out.keys.back(), location, data[out.keys.back()]->type, types::Type::kReference);
    }
    data[out.keys.back()] = this_obj;

    // NOTE: Nothing else left in the objects buffer? Create a new element in the `cfg_res` vector
    // in case we have a duplicate struct later. We won't resolve that now, but later in another
    // pass.
    if (out.objects.empty()) {
      out.cfg_res.emplace_back();
    }

    out.keys.pop_back();
    out.flat_keys.pop_back();
  }
};

template <>
struct action<CBc> {
  static void apply0(ActionData& out) {
    out.depth--;
    CONFIG_ACTION_DEBUG("Depth is now {}", out.depth);
  }
};

}  // namespace flexi_cfg::config

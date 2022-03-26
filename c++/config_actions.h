#pragma once

#include <filesystem>
#include <iostream>
#include <string>
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>
#include <vector>

#include "config_grammar.h"
#include "config_helpers.h"

namespace config {

// TODO: Strip trailing whitespace from comments!

template <typename Rule>
struct selector : peg::parse_tree::selector<
                      Rule,
                      peg::parse_tree::store_content::on<HEX, NUMBER, STRING, VAR, FLAT_KEY, KEY,
                                                         COMMENT, filename::FILENAME>,
                      peg::parse_tree::remove_content::on<END, REF_VARADD, REF_VARSUB, VAR_REF>,
                      peg::parse_tree::fold_one::on<CONFIG, STRUCTc, STRUCT, PROTO, REFERENCE,
                                                    VALUE, PAIR, LIST>> {};

template <>
struct selector<INCLUDE> : std::true_type {
  template <typename... States>
  static void transform(std::unique_ptr<peg::parse_tree::node>& n, States&&... st) {
    std::cout << "Successfully found a " << n->type << " node in " << n->source << "!\n";
    std::string filename = "";
    for (const auto& c : n->children) {
      std::cout << "Has child of type " << c->type << ".\n";
      if (c->has_content()) {
        std::cout << "  Content: " << c->string_view() << "\n";
      }
      if (c->is_type<filename::FILENAME>()) {
        filename = c->string();
      }
    }
    std::cout << std::endl;

    std::cout << " --- Node has " << n->children.size() << " children.\n";

    //// Let's try to parse the "include" file and replace this node with the output!
    // Build the parse path:
    auto include_path = std::filesystem::path(n->source).parent_path() / filename;

    std::cout << "Trying to parse: " << include_path << std::endl;
    auto input_file = peg::file_input(include_path);

    // This is a bit of a hack. We need to use a 'string_input' here because if we don't, the
    // 'file_input' will go out of scope, and the generating the 'dot' output will fail. This would
    // likely be the case when trying to walk the parse tree later.
    // TODO: Find a more elegant way of
    // doing this.
    const auto file_contents = std::string(input_file.begin(), input_file.size());
    auto input = peg::string_input(file_contents, input_file.source());

    // TODO: Convert this to a `parse_nested` call, which will likely fix the issues noted above of
    // requiring a string input instead of a file input.
    // See: https://github.com/taocpp/PEGTL/blob/main/doc/Inputs-and-Parsing.md#nested-parsing

    if (auto new_tree = peg::parse_tree::parse<config::grammar, config::selector>(input)) {
#if PRINT_DOT
      peg::parse_tree::print_dot(std::cout, *new_tree->children.back());
#endif
      auto c = std::move(new_tree->children.back());
      std::cout << " --- Parsed include file has " << c->children.size() << " children.\n";
      new_tree->children.pop_back();
      n->remove_content();
      n = std::move(c);
      std::cout << "c=" << c.get() << std::endl;
      std::cout << "n=" << n.get() << std::endl;
      std::cout << " --- n has " << n->children.size() << " children." << std::endl;
      std::cout << " --- Source: " << n->source << std::endl;
      std::cout << " ------ size: " << n->source.size() << std::endl;
    } else {
      std::cout << "Failed to properly parse " << include_path << "." << std::endl;
    }
  }
};

template <typename Rule>
struct action : peg::nothing<Rule> {};

// Add action to perform when a `proto` is encountered!
struct ActionData {
  std::vector<std::string> proto_names{};
  std::vector<std::string> struct_names{};
  std::vector<std::string> reference_names{};

  int depth{0};  // Nesting level

  std::string result;
  std::vector<std::string> keys;
  std::vector<std::string> flat_keys;

  std::vector<std::pair<std::string, std::string>> pairs;

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
    std::cout << "result: " << result << std::endl;
    std::cout << "^^^^^^^^^^" << std::endl;
  }
};

template <>
struct action<KEY> {
  template <typename ActionInput>
  static void apply(const ActionInput& in, ActionData& out) {
    // std::cout << "Found key: '" << in.string() << "'" << std::endl;
    out.keys.emplace_back(in.string());
  }
};

template <>
struct action<VALUE> {
  template <typename ActionInput>
  static void apply(const ActionInput& in, ActionData& out) {
    // std::cout << "Found value: " << in.string() << std::endl;
    out.result = in.string();
  }
};

template <>
struct action<FLAT_KEY> {
  template <typename ActionInput>
  static void apply(const ActionInput& in, ActionData& out) {
    // A FLAT_KEY is made up of a bunch of KEYs, but we may not want to consume all keys here
    // (because this could be inside another struct. Nominally, a FLAT_KEY shouldn't be in a
    // struct/proto/reference, but a FLAT_KEY can be used to represent a variable reference, so it
    // might be.
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
      std::cout << std::string(out.depth * 2, ' ') << "Popping: " << keys.back() << " | "
                << out.keys.back() << std::endl;
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
    // This is a bit hacky. We need to look at what was parsed, and pull the correct keys out of the
    // existing key list.
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
    // std::cout << out.keys.back() << " = " << out.result << std::endl;

    out.pairs.push_back({out.keys.back(), out.result});
    out.keys.pop_back();
    out.result = "";
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

    out.pairs.push_back({out.flat_keys.back(), out.result});
    out.flat_keys.pop_back();
    out.result = "";
  }
};

template <>
struct action<STRUCTs> {
  static void apply0(ActionData& out) {
    out.depth++;
    std::cout << std::string(out.depth * 2, ' ') << "Depth is now " << out.depth << std::endl;
  }
};

template <>
struct action<PROTOs> {
  static void apply0(ActionData& out) {
    out.depth++;
    std::cout << std::string(out.depth * 2, ' ') << "Depth is now " << out.depth << std::endl;
  }
};

template <>
struct action<REFs> {
  static void apply0(ActionData& out) {
    out.depth++;
    std::cout << std::string(out.depth * 2, ' ') << "Depth is now " << out.depth << std::endl;
  }
};

template <>
struct action<STRUCT> {
  static void apply0(ActionData& out) {
    // std::cout << "Found Struct: " << out.keys.back() << std::endl;
    // TODO: This isn't really what we want to do, but it's simple enough for now.
    out.struct_names.emplace_back(out.keys.back());
    out.keys.pop_back();
  }
};

template <>
struct action<PROTO> {
  static void apply0(ActionData& out) {
    // std::cout << "Found Proto: " << out.keys.back() << std::endl;
    // TODO: This isn't really what we want to do, but it's simple enough for now.
    out.proto_names.emplace_back(out.keys.back());
    out.keys.pop_back();
  }
};

template <>
struct action<REFERENCE> {
  static void apply0(ActionData& out) {
    // std::cout << "Found Struct: " << out.keys.back() << std::endl;
    // TODO: This isn't really what we want to do, but it's simple enough for now.
    out.reference_names.emplace_back(out.keys.back());
    out.keys.pop_back();
  }
};

template <>
struct action<END> {
  static void apply0(ActionData& out) {
    // TODO: If we've found an end tag, then the last two keys should match. If they don't, we've
    // made a mistake!
    if (out.keys.size() < 2) {
      std::cerr << "[END] This is bad. Probably should throw here." << std::endl;
      return;
    }
    const auto end_key = out.keys.back();
    out.keys.pop_back();
    std::cout << std::string(out.depth * 2, ' ') << "End key matches struct key? "
              << (end_key == out.keys.back()) << std::endl;
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

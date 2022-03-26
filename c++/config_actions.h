#pragma once

#include <filesystem>
#include <iostream>
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>

#include "config_grammar.h"

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

    if (auto new_tree = peg::parse_tree::parse<config::grammar, config::selector>(input)) {
      peg::parse_tree::print_dot(std::cout, *new_tree->children.back());
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

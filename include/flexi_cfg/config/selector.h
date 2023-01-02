#pragma once

#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>

#include "flexi_cfg/config/grammar.h"
#include "flexi_cfg/logger.h"

namespace flexi_cfg::config {
// TODO(miker2): Strip trailing whitespace from comments!

template <typename Rule>
struct selector
    : peg::parse_tree::selector<
          Rule,
          peg::parse_tree::store_content::on<HEX, NUMBER, STRING, VAR, FLAT_KEY, KEY, COMMENT,
                                             filename::FILENAME>,
          peg::parse_tree::remove_content::on<END, REF_ADDKVP, REF_VARDEF, VALUE_LOOKUP>,
          peg::parse_tree::fold_one::on<CONFIG, STRUCTc, STRUCT, PROTO, REFERENCE, VALUE, PAIR,
                                        LIST>> {};

template <>
struct selector<INCLUDE> : std::true_type {
  template <typename... States>
  static void transform(std::unique_ptr<peg::parse_tree::node>& n, States&&... st) {
    logger::info("Successfully found a {} node in {}!", n->type, n->source);
    std::string filename{};
    for (const auto& c : n->children) {
      logger::debug("Has child of type {}.", c->type);
      if (c->has_content()) {
        logger::debug("  Content: {}", c->string_view());
      }
      if (c->is_type<filename::FILENAME>()) {
        filename = c->string();
      }
    }
    std::cout << std::endl;

    logger::debug(" --- Node has {} children.", n->children.size());

    //// Let's try to parse the "include" file and replace this node with the output!
    // Build the parse path:
    auto include_path = std::filesystem::path(n->source).parent_path() / filename;

    logger::debug("Trying to parse: {}", include_path.string());
    auto input_file = peg::file_input(include_path);

    // This is a bit of a hack. We need to use a 'string_input' here because if we don't, the
    // 'file_input' will go out of scope, and the generating the 'dot' output will fail. This would
    // likely be the case when trying to walk the parse tree later.
    // TODO(miker2): Find a more elegant way of doing this.
    const auto file_contents = std::string(input_file.begin(), input_file.size());
    auto input = peg::string_input(file_contents, input_file.source());

    // TODO(miker2): Convert this to a `parse_nested` call, which will likely fix the issues
    // noted above of requiring a string input instead of a file input. See:
    // https://github.com/taocpp/PEGTL/blob/main/doc/Inputs-and-Parsing.md#nested-parsing Problem
    // is, the parse_tree doesn't have a parse_nested.

    if (auto new_tree = peg::parse_tree::parse<config::grammar, config::selector>(input)) {
#if PRINT_DOT
      peg::parse_tree::print_dot(std::cout, *new_tree->children.back());
#endif
      auto c = std::move(new_tree->children.back());
      logger::trace(" --- Parsed include file has {} children.", c->children.size());
      new_tree->children.pop_back();
      n->remove_content();
      n = std::move(c);
      logger::trace("c={}", fmt::ptr(c));
      logger::trace(
          "n={}\n"
          " --- n has {} children.\n"
          " --- Source: {}\n"
          " ------ size: {}",
          fmt::ptr(n), n->children.size(), n->source, n->source.size());
    } else {
      logger::error("Failed to properly parse {}", include_path.string());
    }
  }
};

}  // namespace config

#pragma once

#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>

#include "config_grammar.h"

namespace config {

// TODO: Strip trailing whitespace from comments!

template <typename Rule>
using selector = peg::parse_tree::selector<
    Rule,
    peg::parse_tree::store_content::on<HEX, NUMBER, STRING, VAR, FLAT_KEY, KEY, COMMENT,
                                       filename::FILENAME>,
    peg::parse_tree::remove_content::on<END, VARADD, VARREF, INCLUDE>,
    /*
      peg::parse_tree::remove_content::on<WS_, NL, SP, oSP, COMMENT, TAIL,
      COMMA, SBo, SBc, CBo, CBc, KVs, HEXTAG, sign, exp>,
    */
    peg::parse_tree::fold_one::on<CONFIG, STRUCTc, STRUCT, PROTO, REFERENCE, VALUE, PAIR, LIST>>;

template <typename Rule>
struct action : peg::nothing<Rule> {};

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

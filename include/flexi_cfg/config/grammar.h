#pragma once

#include <tao/pegtl.hpp>

namespace peg = TAO_PEGTL_NAMESPACE;

// The end goal of this is to be able to take a string of text (or a file)
// And create a structured tree of data.
// The data should consist only of specific types and eliminate all other
// unecessary data (e.g. whitespace, etc).

// This grammar parses a filename (path + filename) corresponding to a cfg file.
namespace filename {

struct DOTDOT : peg::two<'.'> {};
struct EXT : TAO_PEGTL_KEYWORD(".cfg") {};
struct SEP : peg::one<'/'> {};

// There may be other valid characters in a filename. What might they be?
struct ALPHAPLUS : peg::plus<peg::sor<peg::identifier_other, peg::one<'-'>>> {};

struct FILEPART : peg::sor<DOTDOT, ALPHAPLUS> {};
struct FILENAME : peg::seq<peg::list<FILEPART, SEP>, EXT> {};

struct grammar : peg::must<FILENAME> {};

}  // namespace filename

// Pre-declare the top level rule for the math grammar here.
namespace math {
struct expression;
}

namespace config {

// TODO: Update this
// clang-format off
/*
grammar my_config
  map        <-  _ (struct / proto / reference / COMMENT)+
  struct     <-  STRUCTs KEY TAIL STRUCTc END _
  proto      <-  PROTOs KEY TAIL STRUCTc END _
  reference  <-  REFs FLAT_KEY _ "as" _ KEY TAIL REFc END _
  STRUCTs    <-  "struct" SP
  PROTOs     <-  "proto" SP
  REFs       <-  "reference" SP
  END        <-  "end" SP KEY
  STRUCTc    <-  (struct / PAIR / reference / proto)+
  REFc       <-  (REF_VARDEF / REF_ADDKVP)+
  PAIR       <-  KEY KVs (value / VALUE_LOOKUP) TAIL
  REF_VARDEF <-  VAR KVs value TAIL
  REF_ADDKVP <-  "+" KEY KVs value TAIL
  FLAT_KEY   <-  KEY ("." KEY)+   # Flattened struct/reference syntax
  KEY        <-  [a-z] [a-zA-Z0-9_]*
  value      <-  list / HEX / number / string
  string     <-  '"' [^"]* '"' %make_string
  list       <-  SBo value (COMMA value)* SBc
  number     <-  (!HEX) [+-]? [0-9]+ ("." [0-9]*)? ("e" [+-]? [0-9]+)?
  VARc       <-  [A-Z] [A-Z0-9_]*
  VAR        <-  "$" ("{" VARc "}" / VARc)
  VALUE_LOOKUP <-  "$(" FLAT_KEY ")"
  HEX        <-  "0" [xX] [0-9a-fA-F]+
  KVs        <-  oSP "=" oSP
  CBo        <-  "{" oSP
  CBc        <- oSP "}" _
  SBo        <-  "[" oSP
  SBc        <-  oSP "]"
  COMMA      <-  oSP "," oSP
  TAIL       <-  _ (COMMENT)*
  COMMENT    <-  "#" [^\n\r]* _
  oSP        <-  [ \t]*      # optional space
  SP         <-  [ \t]+      # mandatory space
  NL         <-  [\r\n]+     # (required) new line
  _          <-  [ \t\r\n]*  # All whitespace
*/
// clang-format on

struct WS_ : peg::star<peg::space> {};
struct NL : peg::plus<peg::eol> {};
struct SP : peg::plus<peg::blank> {};
struct oSP : peg::star<peg::blank> {};
struct COMMENT : peg::seq<peg::one<'#'>, peg::until<peg::eol>, WS_> {};
struct TAIL : peg::seq<WS_, peg::star<COMMENT>> {};
// A rule for padding another rule with blanks on either side
template <typename Rule>
struct pd : peg::pad<Rule, peg::blank> {};

struct COMMA : pd<peg::one<','>> {};
// Square brackets
struct SBo : pd<peg::one<'['>> {};
struct SBc : pd<peg::one<']'>> {};
// Curly braces
struct CBo : pd<peg::one<'{'>> {};
struct CBc : pd<peg::one<'}'>> {};
struct KVs : pd<peg::one<'='>> {};  // Key/value separator
// These two rules define the enclosing delimiters for a math expression (i.e. "{{" and "}}")
struct Eo : pd<peg::two<'{'>> {};
struct Ec : pd<peg::two<'}'>> {};

// These are reserved keywords that can't be used elsewhere
struct STRUCTk : TAO_PEGTL_KEYWORD("struct") {};
struct PROTOk : TAO_PEGTL_KEYWORD("proto") {};
struct REFk : TAO_PEGTL_KEYWORD("reference") {};
struct ASk : TAO_PEGTL_KEYWORD("as") {};
struct PARENTNAMEk : TAO_PEGTL_KEYWORD("$PARENT_NAME") {};

struct RESERVED : peg::sor<STRUCTk, PROTOk, REFk, ASk> {};

struct HEXTAG : peg::seq<peg::one<'0'>, peg::one<'x', 'X'>> {};
struct HEX : peg::seq<HEXTAG, peg::plus<peg::xdigit>> {};

struct sign : peg::one<'+', '-'> {};
struct exp : peg::seq<peg::one<'e', 'E'>, peg::opt<sign>, peg::plus<peg::digit>> {};

// NOTE: We want to use the same common part for an "INTEGER" and a "FLOAT", but we don't want the
// "INTEGER" sub-rule to match when looking at the beginning of a float, so we do this trick here.
// "INTEGER_" is the same for both an "INTEGER" and a "FLOAT", but when matching an "INTEGER" we
// ensure that there is no decimal point at the end.
struct INTEGER_
    : peg::seq<peg::opt<sign>,
               peg::sor<peg::one<'0'>, peg::seq<peg::range<'1', '9'>, peg::star<peg::digit>>>> {};
struct INTEGER : peg::seq<INTEGER_, peg::not_at<peg::one<'.'>>> {};
struct FLOAT
    : peg::seq<INTEGER_,
               peg::sor<peg::seq<peg::one<'.'>, peg::star<peg::digit>, peg::opt<exp>>, exp>> {};
struct NUMBER : peg::sor<FLOAT, INTEGER> {};

struct TRUE : TAO_PEGTL_KEYWORD("true") {};
struct FALSE : TAO_PEGTL_KEYWORD("false") {};
struct BOOLEAN : peg::sor<TRUE, FALSE> {};

struct STRING : peg::seq<peg::one<'"'>, peg::plus<peg::not_one<'"'>>, peg::one<'"'>> {};

struct LIST;
struct VALUE : peg::sor<HEX, NUMBER, STRING, BOOLEAN, LIST> {};
// 'seq' is used here so that the 'VALUE' action will collect the location information.
struct LIST_ELEMENT : peg::seq<VALUE> {};
// Should the 'space' here be a 'blank'? Allow multi-line lists (w/o \)?
struct LIST : peg::seq<SBo, peg::list<LIST_ELEMENT, COMMA, peg::space>, SBc> {
  using begin = SBo;
  using end = SBc;
  using element = LIST_ELEMENT;
};

struct EXPRESSION : peg::seq<Eo, math::expression, Ec> {};

// Account for all reserved keywords when looking for keys
struct KEY : peg::seq<peg::not_at<RESERVED>, peg::lower, peg::star<peg::identifier_other>> {};
struct FLAT_KEY : peg::list<KEY, peg::one<'.'>> {};

// A 'VAR' can only be found within a proto (and it's children)
struct VARc : peg::seq<peg::upper, peg::star<peg::ranges<'A', 'Z', '0', '9', '_'>>> {};
// Allow for VAR to be expessed as: $VAR or ${VAR}
struct VAR : peg::seq<peg::one<'$'>, peg::sor<peg::seq<peg::one<'{'>, VARc, peg::one<'}'>>, VARc>> {
};

struct VALUE_LOOKUP : peg::seq<TAO_PEGTL_STRING("$("), peg::list<peg::sor<KEY, VAR>, peg::one<'.'>>,
                               peg::one<')'>> {};

struct REF_ADDKVP
    : peg::seq<peg::one<'+'>, KEY, KVs, peg::sor<VALUE, VALUE_LOOKUP, EXPRESSION>, TAIL> {};
struct REF_VARDEF
    : peg::seq<VAR, KVs, peg::sor<VALUE, VALUE_LOOKUP, EXPRESSION, PARENTNAMEk>, TAIL> {};

struct FULLPAIR : peg::seq<FLAT_KEY, KVs, peg::sor<VALUE, VALUE_LOOKUP, EXPRESSION>, TAIL> {};
struct PAIR : peg::seq<KEY, KVs, peg::sor<VALUE, VALUE_LOOKUP, EXPRESSION>, TAIL> {};
struct PROTO_PAIR : peg::seq<KEY, KVs, peg::sor<VALUE, VALUE_LOOKUP, EXPRESSION, VAR>, TAIL> {};

struct END : CBc {};

struct REFs : peg::seq<REFk, SP, FLAT_KEY, SP, ASk, SP, KEY, CBo, TAIL> {};
struct REFc : peg::plus<peg::sor<REF_VARDEF, REF_ADDKVP>> {};
struct REFERENCE : peg::seq<REFs, REFc, END, TAIL> {};

struct PROTOc;
struct PROTOs : peg::seq<PROTOk, SP, KEY, CBo, TAIL> {};
struct PROTO : peg::seq<PROTOs, PROTOc, END, TAIL> {};

struct STRUCTc;
struct STRUCTs : peg::seq<STRUCTk, SP, KEY, CBo, TAIL> {};
struct STRUCT : peg::seq<STRUCTs, STRUCTc, END, TAIL> {};

// Special definition of a struct contained in a proto
struct STRUCT_IN_PROTOc;
struct STRUCT_IN_PROTO : peg::seq<STRUCTs, PROTOc, END, TAIL> {};

struct PROTOc : peg::plus<peg::sor<PROTO_PAIR, STRUCT_IN_PROTO, REFERENCE>> {};
struct STRUCTc : peg::plus<peg::sor<PAIR, STRUCT, REFERENCE, PROTO>> {};

// Include syntax
struct INCLUDE : peg::seq<TAO_PEGTL_KEYWORD("include"), SP, filename::grammar, TAIL> {};
struct include_list : peg::star<INCLUDE> {};

// A single file should look like this:
//
//  1. Optional list of include files
//  2. Elements of a config file: flat keys OR struct / proto / reference / pair

// The `peg::sor<...>` here matches one or more `FULLPAIR` objects or the contents of a `STRUCT`,
// which includes the following items:
//  * STRUCT
//  * REFERENCE
//  * PROTO
//  * PAIR
//
// but never both in the same file. The `peg::not_at<PAIR>` prevents the PAIR that might appear in a
// `STRUCTc` from being matched as a `FULLPAIR` object.

struct CONFIG
    : peg::seq<TAIL, include_list,
               peg::sor<peg::seq<peg::not_at<PAIR>, peg::plus<FULLPAIR>>, STRUCTc>, TAIL> {};

struct grammar : peg::seq<CONFIG, peg::eolf> {};

}  // namespace config

#include "flexi_cfg/math/grammar.h"

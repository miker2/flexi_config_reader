#pragma once

#include <tao/pegtl.hpp>

namespace peg = TAO_PEGTL_NAMESPACE;

// The end goal of this is to be able to take a string of text (or a file)
// And create a structured tree of data.
// The data should consist only of specific types and eliminate all other
// unecessary data (e.g. whitespace, etc).

namespace config {

/*
  TODO: Add validator to throw on duplicate keys
 */

/*
# TODO: Support the following syntax:
#         key = $(path.to.other.key)
#       Where $(path.to.other.key) references the value in another struct

# TODO: Support `FLAT_KEY KVs value` type entries (user can specify fully
qualified values)

# TODO: Create example with more comments to ensure they're handled properly
everywhere.

grammar my_config
  map       <-  _ (struct / proto / reference)+ _ %make_map
  struct    <-  STRUCTs KEY TAIL STRUCTc END _ %make_struct
  proto     <-  PROTOs KEY TAIL STRUCTc END _ %make_proto
  reference <-  REFs FLAT_KEY _ "as" _ KEY TAIL REFc END _ %make_reference
  STRUCTs   <-  "struct" SP
  PROTOs    <-  "proto" SP
  REFs      <-  "reference" SP
  END       <-  "end" SP KEY
  STRUCTc   <-  (struct / PAIR / reference / proto)+
  REFc      <-  (VARREF / VARADD)+
  PAIR      <-  KEY KVs (value / VAR) TAIL %make_pair
  VARREF    <-  VAR KVs value TAIL %ref_sub_var
  VARADD    <-  "+" KEY KVs value TAIL %ref_add_var
  FLAT_KEY  <-  KEY ("." KEY)?  %found_key  # Flattened struct/reference syntax
  KEY       <-  [a-z] [a-zA-Z0-9_]*  %found_key
  value     <-  list / HEX / number / string
  string    <-  '"' [^"]* '"' %make_string
  list      <-  SBo value (COMMA value)* SBc %make_list
  number    <-  (!HEX) [+-]? [0-9]+ ("." [0-9]*)? ("e" [+-]? [0-9]+)? %make_number
  VAR       <-  "$" [A-Z0-9_]+  %make_var
  HEX       <-  "0" [xX] [0-9a-fA-F]+ %make_hex
  KVs       <-  oSP "=" oSP
  CBo       <-  "{" oSP
  CBc       <- oSP "}" _
  SBo       <-  "[" oSP
  SBc       <-  oSP "]"
  COMMA     <-  oSP "," oSP
  TAIL      <-  _ (COMMENT)*
  COMMENT   <-  "#" [^\n\r]* _
  oSP       <-  [ \t]*      # optional space
  SP        <-  [ \t]+      # mandatory space
  NL        <-  [\r\n]+     # (required) new line
  _         <-  [ \t\r\n]*  # All whitespace
*/

struct WS_ : peg::star<peg::space> {};
struct NL : peg::plus<peg::eol> {};
struct SP : peg::plus<peg::blank> {};
struct oSP : peg::star<peg::blank> {};
struct COMMENT : peg::seq<peg::one<'#'>, peg::until<peg::eol>, WS_> {};
struct TAIL : peg::seq<WS_, peg::star<COMMENT>> {};
struct COMMA : peg::pad<peg::one<','>, peg::blank> {};
struct SBo : peg::pad<peg::one<'['>, peg::blank> {};
struct SBc : peg::pad<peg::one<']'>, peg::blank> {};
struct CBo : peg::pad<peg::one<'{'>, peg::blank> {};
struct CBc : peg::pad<peg::one<'}'>, peg::blank> {};
struct KVs : peg::pad<peg::one<'='>, peg::blank> {};

struct HEXTAG : peg::seq<peg::one<'0'>, peg::one<'x', 'X'>> {};
struct HEX : peg::seq<HEXTAG, peg::plus<peg::xdigit>> {};

struct VAR
    : peg::seq<peg::one<'$'>, peg::plus<peg::ranges<'A', 'Z', '0', '9', '_'>>> {
};

struct sign : peg::one<'+', '-'> {};
struct exp
    : peg::seq<peg::one<'e', 'E'>, peg::opt<sign>, peg::plus<peg::digit>> {};
struct INTEGER
    : peg::seq<peg::opt<sign>,
               peg::sor<peg::one<'0'>, peg::seq<peg::range<'1', '9'>,
                                                peg::star<peg::digit>>>> {};
struct FLOAT
    : peg::seq<INTEGER, peg::one<'.'>, peg::star<peg::digit>, peg::opt<exp>> {};
struct NUMBER : peg::sor<FLOAT, INTEGER> {};

struct STRING
    : peg::seq<peg::one<'"'>, peg::plus<peg::not_one<'"'>>, peg::one<'"'>> {};

struct VALUE;
// Should the 'space' here be a 'blank'? Allow multi-line lists (w/o \)?
struct LIST : peg::seq<SBo, peg::list<VALUE, COMMA, peg::space>, SBc> {};

struct VALUE : peg::sor<LIST, HEX, NUMBER, STRING> {};

struct KEY
    : peg::seq<peg::range<'a', 'z'>,
               peg::star<peg::ranges<'a', 'z', 'A', 'Z', '0', '9', '_'>>> {};
struct FLAT_KEY : peg::list<KEY, peg::one<'.'>> {};
struct VARADD : peg::seq<peg::one<'+'>, KEY, KVs, VALUE, TAIL> {};
struct VARREF : peg::seq<VAR, KVs, VALUE, TAIL> {};

struct PAIR : peg::seq<KEY, KVs, peg::sor<VALUE, VAR>, TAIL> {};

struct END : peg::seq<peg::keyword<'e', 'n', 'd'>, SP, KEY> {};

struct REFs : peg::seq<TAO_PEGTL_KEYWORD("reference"), SP> {};
struct REFc : peg::plus<peg::sor<VARREF, VARADD>> {};
struct REFERENCE : peg::seq<REFs, FLAT_KEY, WS_, peg::keyword<'a', 's'>, WS_,
                            KEY, TAIL, REFc, END, WS_> {};

struct STRUCTc;
struct PROTOs : peg::seq<TAO_PEGTL_KEYWORD("proto"), SP> {};
struct PROTO : peg::seq<PROTOs, KEY, TAIL, STRUCTc, END, WS_> {};

struct STRUCTs : peg::seq<TAO_PEGTL_KEYWORD("struct"), SP> {};
struct STRUCT : peg::seq<STRUCTs, KEY, TAIL, STRUCTc, END, WS_> {};

struct STRUCTc : peg::plus<peg::sor<STRUCT, PAIR, REFERENCE, PROTO>> {};

struct CONFIG
    : peg::seq<WS_, peg::plus<peg::sor<STRUCT, PROTO, REFERENCE>>, WS_> {};

struct grammar : peg::seq<CONFIG, peg::eolf> {};

}

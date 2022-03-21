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
struct ALPHAPLUS
    : peg::plus<peg::sor<peg::ranges<'A', 'Z', 'a', 'z', '0', '9', '_'>, peg::one<'-'>>> {};

struct FILEPART : peg::sor<DOTDOT, ALPHAPLUS> {};
struct FILENAME : peg::seq<peg::list<FILEPART, SEP>, EXT> {};

struct grammar : peg::must<FILENAME> {};

}  // namespace filename

namespace config {

/*
  TODO: Add validator to throw on duplicate keys
 */

// clang-format off
/*
grammar my_config
  map        <-  _ (struct / proto / reference)+ _ %make_map
  struct     <-  STRUCTs KEY TAIL STRUCTc END _ %make_struct
  proto      <-  PROTOs KEY TAIL STRUCTc END _ %make_proto
  reference  <-  REFs FLAT_KEY _ "as" _ KEY TAIL REFc END _ %make_reference
  STRUCTs    <-  "struct" SP
  PROTOs     <-  "proto" SP
  REFs       <-  "reference" SP
  END        <-  "end" SP KEY
  STRUCTc    <-  (struct / PAIR / reference / proto)+
  REFc       <-  (VARREF / VARADD)+
  PAIR       <-  KEY KVs (value / VAR_REF / VAR) TAIL %make_pair
  REF_VARSUB <-  VAR KVs value TAIL %ref_sub_var
  REF_VARADD <-  "+" KEY KVs value TAIL %ref_add_var
  FLAT_KEY   <-  KEY ("." KEY)+  %found_key  # Flattened struct/reference syntax
  KEY        <-  [a-z] [a-zA-Z0-9_]*  %found_key
  value      <-  list / HEX / number / string
  string     <-  '"' [^"]* '"' %make_string
  list       <-  SBo value (COMMA value)* SBc %make_list
  number     <-  (!HEX) [+-]? [0-9]+ ("." [0-9]*)? ("e" [+-]? [0-9]+)? %make_number
  VAR        <-  "$" [A-Z0-9_]+  %make_var
  VAR_REF    <-  "$(" FLAT_KEY ")" %var_ref
  HEX        <-  "0" [xX] [0-9a-fA-F]+ %make_hex
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
struct COMMA : peg::pad<peg::one<','>, peg::blank> {};
struct SBo : peg::pad<peg::one<'['>, peg::blank> {};
struct SBc : peg::pad<peg::one<']'>, peg::blank> {};
struct CBo : peg::pad<peg::one<'{'>, peg::blank> {};
struct CBc : peg::pad<peg::one<'}'>, peg::blank> {};
struct KVs : peg::pad<peg::one<'='>, peg::blank> {};

struct HEXTAG : peg::seq<peg::one<'0'>, peg::one<'x', 'X'>> {};
struct HEX : peg::seq<HEXTAG, peg::plus<peg::xdigit>> {};

struct VAR : peg::seq<peg::one<'$'>, peg::plus<peg::ranges<'A', 'Z', '0', '9', '_'>>> {};

struct sign : peg::one<'+', '-'> {};
struct exp : peg::seq<peg::one<'e', 'E'>, peg::opt<sign>, peg::plus<peg::digit>> {};
struct INTEGER
    : peg::seq<peg::opt<sign>,
               peg::sor<peg::one<'0'>, peg::seq<peg::range<'1', '9'>, peg::star<peg::digit>>>> {};
struct FLOAT : peg::seq<INTEGER, peg::one<'.'>, peg::star<peg::digit>, peg::opt<exp>> {};
struct NUMBER : peg::sor<FLOAT, INTEGER> {};

struct STRING : peg::seq<peg::one<'"'>, peg::plus<peg::not_one<'"'>>, peg::one<'"'>> {};

struct VALUE;
// Should the 'space' here be a 'blank'? Allow multi-line lists (w/o \)?
struct LIST : peg::seq<SBo, peg::list<VALUE, COMMA, peg::space>, SBc> {};

struct VALUE : peg::sor<LIST, HEX, NUMBER, STRING> {};

struct KEY
    : peg::seq<peg::range<'a', 'z'>, peg::star<peg::ranges<'a', 'z', 'A', 'Z', '0', '9', '_'>>> {};
struct FLAT_KEY : peg::list<KEY, peg::one<'.'>> {};
struct REF_VARADD : peg::seq<peg::one<'+'>, KEY, KVs, VALUE, TAIL> {};
struct REF_VARSUB : peg::seq<VAR, KVs, VALUE, TAIL> {};

struct VAR_REF : peg::seq<TAO_PEGTL_STRING("$("), FLAT_KEY, peg::one<')'>> {};

struct PAIR : peg::seq<KEY, KVs, peg::sor<VALUE, VAR_REF, VAR>, TAIL> {};
struct FULLPAIR : peg::seq<FLAT_KEY, KVs, peg::sor<VALUE, VAR_REF>, TAIL> {};

struct END : peg::seq<TAO_PEGTL_KEYWORD("end"), SP, KEY> {};

struct REFs : peg::seq<TAO_PEGTL_KEYWORD("reference"), SP> {};
struct REFc : peg::plus<peg::sor<REF_VARSUB, REF_VARADD>> {};
struct REFERENCE
    : peg::seq<REFs, FLAT_KEY, WS_, peg::keyword<'a', 's'>, WS_, KEY, TAIL, REFc, END, WS_> {};

struct STRUCTc;
struct PROTOs : peg::seq<TAO_PEGTL_KEYWORD("proto"), SP> {};
struct PROTO : peg::seq<PROTOs, KEY, TAIL, STRUCTc, END, WS_> {};

struct STRUCTs : peg::seq<TAO_PEGTL_KEYWORD("struct"), SP> {};
struct STRUCT : peg::seq<STRUCTs, KEY, TAIL, STRUCTc, END, WS_> {};

struct STRUCTc : peg::plus<peg::sor<STRUCT, PAIR, REFERENCE, PROTO>> {};

// TODO: Improve this. A single file should look like this:
//
//  1. Optional list of include files
//  2. Elements of a config file: struct, proto, reference, pair
//
// How do we fit in flat keys? I think we want to support flat keys or
// structured keys in a single file. But not both.

struct INCLUDE : peg::seq<TAO_PEGTL_KEYWORD("include"), SP, filename::grammar, TAIL> {};
struct include_list : peg::star<INCLUDE> {};

struct CONFIG : peg::seq<TAIL, include_list, peg::sor<STRUCTc, peg::plus<FULLPAIR>>, TAIL> {};

struct grammar : peg::seq<CONFIG, peg::eolf> {};

}  // namespace config

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

namespace config {

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
  PAIR       <-  KEY KVs (value / VAR_REF) TAIL %make_pair
  REF_VARSUB <-  VAR KVs value TAIL %ref_sub_var
  REF_VARADD <-  "+" KEY KVs value TAIL %ref_add_var
  FLAT_KEY   <-  KEY ("." KEY)+  %found_key  # Flattened struct/reference syntax
  KEY        <-  [a-z] [a-zA-Z0-9_]*  %found_key
  value      <-  list / HEX / number / string
  string     <-  '"' [^"]* '"' %make_string
  list       <-  SBo value (COMMA value)* SBc %make_list
  number     <-  (!HEX) [+-]? [0-9]+ ("." [0-9]*)? ("e" [+-]? [0-9]+)? %make_number
  VARc       <-  [A-Z] [A-Z0-9_]*
  VAR        <-  "$" ("{" VARc "}" / VARc)  %make_var
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

struct STRUCTk : TAO_PEGTL_KEYWORD("struct") {};
struct PROTOk : TAO_PEGTL_KEYWORD("proto") {};
struct REFk : TAO_PEGTL_KEYWORD("reference") {};
struct ASk : TAO_PEGTL_KEYWORD("as") {};
struct ENDk : TAO_PEGTL_KEYWORD("end") {};

struct RESERVED : peg::sor<STRUCTk, PROTOk, REFk, ASk, ENDk> {};

struct HEXTAG : peg::seq<peg::one<'0'>, peg::one<'x', 'X'>> {};
struct HEX : peg::seq<HEXTAG, peg::plus<peg::xdigit>> {};

struct VARc : peg::seq<peg::upper, peg::star<peg::ranges<'A', 'Z', '0', '9', '_'>>> {};
// Allow for VAR to be expessed as: $VAR or ${VAR}
struct VAR : peg::seq<peg::one<'$'>, peg::sor<peg::seq<peg::one<'{'>, VARc, peg::one<'}'>>, VARc>> {
};

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

struct STRING : peg::seq<peg::one<'"'>, peg::plus<peg::not_one<'"'>>, peg::one<'"'>> {};

struct VAL_ : peg::sor<HEX, NUMBER, STRING> {};
// Should the 'space' here be a 'blank'? Allow multi-line lists (w/o \)?
struct LIST : peg::seq<SBo, peg::list<VAL_, COMMA, peg::space>, SBc> {};

struct VALUE : peg::sor<VAL_, LIST> {};

// Account for all reserved keywords when looking for keys
struct KEY : peg::seq<peg::not_at<RESERVED>, peg::lower, peg::star<peg::identifier_other>> {};
struct FLAT_KEY : peg::list<KEY, peg::one<'.'>> {};

struct VAR_REF : peg::seq<TAO_PEGTL_STRING("$("), peg::list<peg::sor<KEY, VAR>, peg::one<'.'>>,
                          peg::one<')'>> {};

struct REF_VARADD : peg::seq<peg::one<'+'>, KEY, KVs, VALUE, TAIL> {};
struct REF_VARSUB : peg::seq<VAR, KVs, peg::sor<VALUE, VAR_REF>, TAIL> {};

struct FULLPAIR : peg::seq<FLAT_KEY, KVs, peg::sor<VALUE, VAR_REF>, TAIL> {};
struct PAIR : peg::seq<KEY, KVs, peg::sor<VALUE, VAR_REF>, TAIL> {};
struct PROTO_PAIR : peg::seq<KEY, KVs, peg::sor<VALUE, VAR_REF, VAR>, TAIL> {};

struct END : peg::seq<ENDk, SP, KEY> {};

struct REFs : peg::seq<REFk, SP, FLAT_KEY, SP, ASk, SP, KEY, TAIL> {};
struct REFc : peg::plus<peg::sor<REF_VARSUB, REF_VARADD>> {};
struct REFERENCE : peg::seq<REFs, REFc, END, TAIL> {};

struct PROTOc;
struct PROTOs : peg::seq<PROTOk, SP, KEY, TAIL> {};
struct PROTO : peg::seq<PROTOs, PROTOc, END, TAIL> {};

struct STRUCTc;
struct STRUCTs : peg::seq<STRUCTk, SP, KEY, TAIL> {};
struct STRUCT : peg::seq<STRUCTs, STRUCTc, END, TAIL> {};

// Special definition of a struct contained in a proto
struct STRUCT_IN_PROTOc;
struct STRUCT_IN_PROTO : peg::seq<STRUCTs, PROTOc, END, WS_> {};

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

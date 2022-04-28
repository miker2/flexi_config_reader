#pragma once

#include <tao/pegtl.hpp>

#include "config_grammar.h"

namespace peg = TAO_PEGTL_NAMESPACE;

// A rule for padding another rule with blanks on either side
template <typename Rule>
struct pd : peg::pad<Rule, peg::blank> {};

struct Um : peg::one<'-'> {};
struct Up : peg::one<'+'> {};
struct Uo : peg::sor<Um, Up> {};

struct Bplus : peg::one<'+'> {};
struct Bminus : peg::one<'-'> {};
struct Bmult : peg::one<'*'> {};
struct Bdiv : peg::one<'/'> {};
struct Bpow : peg::sor<peg::one<'^'>, TAO_PEGTL_STRING("**")> {};
struct Bo : peg::sor<Bpow, Bplus, Bminus, Bmult, Bdiv> {};

struct Po : pd<peg::one<'('>> {};
struct Pc : pd<peg::one<')'>> {};

// Consider other named constants as well.
struct pi : TAO_PEGTL_STRING("pi") {};

struct Mo : pd<TAO_PEGTL_STRING("{{")> {};
struct Mc : pd<TAO_PEGTL_STRING("}}")> {};

struct ignored : peg::space {};

// v includes numbers, variables & var refs
struct v : peg::sor<config::NUMBER, config::VAR, config::VAR_REF> {};

namespace grammar1 {
// Grammar G1:
//    expression --> P {B P}
//    P --> v | "(" expression ")" | U P
//    B --> "+" | "-" | "*" | "/" | "^"
//    U --> "-"

struct expression;
struct BRACKET : peg::seq<Po, expression, Pc> {};
struct atom : peg::sor<v, BRACKET, pi> {};
struct P;
struct P : peg::sor<atom /*v, BRACKET*/, peg::seq<Uo, P>> {};  // <-- recursive rule
struct expression : peg::list<P, Bo, ignored> {};              // <-- Terminal

}  // namespace grammar1

namespace grammar2 {
/*
expression --> T {( "+" | "-" ) T}
T --> F {( "*" | "/" ) F}
F --> P ["^" F]
P --> v | "(" expression ")" | "-" T
*/

struct PM : peg::sor<Bplus, Bminus> {};
struct MD : peg::sor<Bmult, Bdiv> {};

struct F;
struct EXP : peg::seq<Bpow, F> {};
struct P;
struct F : peg::seq<P, peg::opt<EXP>> {};
struct T : peg::seq<F, peg::star<pd<MD>, F>> {};
struct N : peg::seq<Um, T> {};
struct expression;
struct BRACKET : peg::seq<Po, expression, Pc> {};
struct P : peg::sor<pd<v>, pd<pi>, BRACKET, N> {};
struct expression : peg::seq<T, peg::star<pd<PM>, T>> {};  // <-- Terminal

}  // namespace grammar2

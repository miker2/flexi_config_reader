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
//    E --> P {B P}
//    P --> v | "(" E ")" | U P
//    B --> "+" | "-" | "*" | "/" | "^"
//    U --> "-"

struct E;
struct BRACKET : peg::seq<Po, E, Pc> {};
struct atom : peg::sor<v, BRACKET, pi> {};
struct P;
struct P : peg::sor<atom /*v, BRACKET*/, peg::seq<Uo, P>> {};
struct E : peg::list<P, Bo, ignored> {};

}  // namespace grammar1

namespace grammar2 {
/*
E --> T {( "+" | "-" ) T}
T --> F {( "*" | "/" ) F}
F --> P ["^" F]
P --> v | "(" E ")" | "-" T
*/

struct PM : peg::sor<Bplus, Bminus> {};
struct MD : peg::sor<Bmult, Bdiv> {};

struct T;
struct E : peg::seq<T, peg::star<pd<PM>, T>> {};
struct F;
struct T : peg::seq<F, peg::star<pd<MD>, F>> {};
struct EXP : peg::seq<Bpow, F> {};
struct P;
struct F : peg::seq<P, peg::opt<EXP>> {};
struct N : peg::seq<Um, T> {};
struct BRACKET : peg::seq<Po, E, Pc> {};
struct P : peg::sor<pd<v>, pd<pi>, BRACKET, N> {};

}  // namespace grammar2

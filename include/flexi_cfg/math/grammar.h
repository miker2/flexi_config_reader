#pragma once

#include <tao/pegtl.hpp>

#include "flexi_cfg/config/grammar.h"

namespace peg = TAO_PEGTL_NAMESPACE;

namespace math {
/*
 expression --> P {B P}
 P --> v | "(" expression ")" | U P
 B --> "+" | "-" | "*" | "/" | "^"
 U --> "-"
*/

struct Um : peg::one<'-'> {};
struct Up : peg::one<'+'> {};
struct Uo : peg::sor<Um, Up> {};

struct Bplus : peg::one<'+'> {};
struct Bminus : peg::one<'-'> {};
struct Bmult : peg::one<'*'> {};
struct Bdiv : peg::one<'/'> {};
struct Bpow : peg::sor<peg::one<'^'>, TAO_PEGTL_STRING("**")> {};
struct Bo : peg::sor<Bpow, Bplus, Bminus, Bmult, Bdiv> {};

struct Po : config::pd<peg::one<'('>> {};
struct Pc : config::pd<peg::one<')'>> {};

// Consider other named constants as well.
struct pi : TAO_PEGTL_STRING("pi") {};

struct ignored : peg::space {};

// v includes numbers, variables & var refs
struct v : peg::sor<config::NUMBER, config::VAR, config::VAR_REF> {};

struct expression;
struct BRACKET : peg::seq<Po, expression, Pc> {};
struct atom : config::pd<peg::sor<v, BRACKET, pi>> {};
struct P;
struct P : peg::sor<atom /*v, BRACKET*/, peg::seq<Uo, P>> {};  // <-- recursive rule
struct expression : peg::list<P, Bo, ignored> {};              // <-- Terminal

}  // namespace math

namespace grammar2 {
/*
 expression --> T {( "+" | "-" ) T}
 T --> F {( "*" | "/" ) F}
 F --> P ["^" F]
 P --> v | "(" expression ")" | "-" T
*/

struct PM : peg::sor<math::Bplus, math::Bminus> {};
struct MD : peg::sor<math::Bmult, math::Bdiv> {};

struct expression;
struct BRACKET : peg::seq<math::Po, expression, math::Pc> {};
struct N;
struct P : config::pd<peg::sor<math::v, math::pi, BRACKET, N>> {};
struct F;
struct EXP : peg::seq<math::Bpow, F> {};
struct F : peg::seq<P, peg::opt<EXP>> {};
struct T : peg::seq<F, peg::star<config::pd<MD>, F>> {};
struct N : peg::seq<math::Um, T> {};
struct expression : peg::seq<T, peg::star<config::pd<PM>, T>> {};  // <-- Terminal

}  // namespace grammar2

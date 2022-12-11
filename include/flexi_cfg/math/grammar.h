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

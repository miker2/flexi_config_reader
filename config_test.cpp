// Type your code here, or load an example.
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <map>
#include <string>
#include <typeinfo>
#include <variant>
#include <vector>

#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/abnf.hpp>
#include <tao/pegtl/contrib/analyze.hpp>
#include <tao/pegtl/contrib/json.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>
#include <tao/pegtl/contrib/parse_tree_to_dot.hpp>
#include <tao/pegtl/contrib/trace.hpp>

namespace peg = TAO_PEGTL_NAMESPACE;

// The end goal of this is to be able to take a string of text (or a file)
// And create a structured tree of data.
// The data should consist only of specific types and eliminate all other
// unecessary data (e.g. whitespace, etc).

namespace config {

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
  struct    <-  STRUCTs KEY TAIL STRUCTc END KEY _ %make_struct
  proto     <-  PROTOs KEY TAIL STRUCTc END KEY _ %make_proto
  reference <-  REFs FLAT_KEY _ "as" _ KEY TAIL REFc END KEY _ %make_reference
  STRUCTs   <-  "struct" SP
  PROTOs    <-  "proto" SP
  REFs      <-  "reference" SP
  END       <-  "end" SP
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
  number    <-  (!HEX) [+-]? [0-9]+ ("." [0-9]*)? ("e" [+-]? [0-9]+)?
%make_number VAR       <-  "$" [A-Z0-9_]+  %make_var HEX       <-  "0" [xX]
[0-9a-fA-F]+ %make_hex KVs       <-  oSP "=" oSP CBo       <-  "{" oSP CBc <-
oSP "}" _ SBo       <-  "[" oSP SBc       <-  oSP "]" COMMA     <-  oSP "," oSP
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

struct END : peg::seq<peg::keyword<'e', 'n', 'd'>, SP> {};

struct REFs : peg::seq<TAO_PEGTL_KEYWORD("reference"), SP> {};
struct REFc : peg::plus<peg::sor<VARREF, VARADD>> {};
struct REFERENCE : peg::seq<REFs, FLAT_KEY, WS_, peg::keyword<'a', 's'>, WS_,
                            KEY, TAIL, REFc, END, KEY, WS_> {};

struct STRUCTc;
struct PROTOs : peg::seq<TAO_PEGTL_KEYWORD("proto"), SP> {};
struct PROTO : peg::seq<PROTOs, KEY, TAIL, STRUCTc, END, KEY, WS_> {};

struct STRUCTs : peg::seq<TAO_PEGTL_KEYWORD("struct"), SP> {};
struct STRUCT : peg::seq<STRUCTs, KEY, TAIL, STRUCTc, END, KEY, WS_> {};

struct STRUCTc : peg::plus<peg::sor<STRUCT, PAIR, REFERENCE, PROTO>> {};

struct CONFIG
    : peg::seq<WS_, peg::plus<peg::sor<STRUCT, PROTO, REFERENCE>>, WS_> {};

struct grammar : peg::seq<CONFIG, peg::eolf> {};

template <typename Rule>
using selector = peg::parse_tree::selector<
    Rule, // peg::parse_tree::store_content::on<HEX, NUMBER, STRING, VAR>,
    peg::parse_tree::remove_content::on<WS_, NL, SP, oSP, COMMENT, TAIL, COMMA,
                                        SBo, SBc, CBo, CBc, KVs, HEXTAG, sign,
                                        exp>,
    peg::parse_tree::fold_one::on<CONFIG, STRUCTc, STRUCT, PROTO, REFERENCE,
                                  VALUE, PAIR, LIST>>;

template <typename Rule> struct action : peg::nothing<Rule> {};

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
} // namespace config

template <typename GTYPE>
auto runTest(size_t idx, const std::string &test_str, bool pdot = true)
    -> bool {
  std::cout << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n";
  std::cout << "Parsing example " << idx << ":\n";
  std::cout << test_str << std::endl;
  std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";

  const auto source = "example " + std::to_string(idx);
  peg::memory_input in(test_str, source);

  bool ret{false};
  std::string out;
  try {
    if (const auto root = peg::parse_tree::parse<GTYPE, config::selector>(in)) {
      if (pdot) {
        peg::parse_tree::print_dot(std::cout, *root);
      }
      std::cout << "  Parse tree success.\n";
    } else {
      std::cout << "  Parse tree failure!\n";
      in.restart();
      peg::standard_trace<GTYPE>(in);
    }
    // Re-use of an input is highly discouraged, but given that we probably
    // won't call both `parse_tree::parse` and `parse` in the same run, we'll
    // re-use the input here for convenience.
    in.restart();
    ret = peg::parse<GTYPE, config::action>(in, out);
    std::cout << "  Parse " << (ret ? "success" : "failure") << std::endl;
    std::cout << "out: " << out << std::endl;
  } catch (const peg::parse_error &e) {
    std::cout << "!!!\n";
    std::cout << "  Parser failure!\n";
    const auto p = e.positions().front();
    std::cout << e.what() << '\n'
              << in.line_at(p) << '\n'
              << std::setw(p.column) << '^' << '\n';
    std::cout << "!!!\n";
  }

  std::cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n";
  return ret;
}

auto main() -> int {

  const bool pdot = false;

  if (peg::analyze<config::grammar>() != 0) {
    std::cout << "Something in the grammar is broken!" << std::endl;
  }

  size_t test_num = 1;
  {
    std::string content = "-1001";
    runTest<peg::must<config::INTEGER, peg::eolf>>(test_num++, content, pdot);
  }
  {
    std::string content = "float.value  =  5.37e+6";
    runTest<peg::must<config::PAIR, peg::eolf>>(test_num++, content, pdot);
  }
  {
    std::string content = "1234.";
    runTest<peg::must<config::NUMBER, peg::eolf>>(test_num++, content, pdot);
  }
  {
    std::string content = "0x0ab0";
    runTest<peg::must<config::VALUE, peg::eolf>>(test_num++, content, pdot);
  }

  std::vector config_strs = {"\n\
struct test1\n\
    key1 = \"value\"\n\
    key2 = 1.342    # test comment here\n\
    key3 = 10\n\
    f = \"none\"\n\
end test1\n\
\n\
struct test2\n\
    my_key = \"foo\"  \n\
    n_key = 1\n\
end test2\n\
",
                             "\n\
struct test1\n\
    key1 = \"value\"\n\
    key2 = 1.342    # test comment here \n\
    key3 = 10\n\
    f = \"none\" \n\
end test1\n\
\n\
struct test2\n\
    my_key = \"foo\"    \n\
    n_key = 1\n\
\n\
    struct inner\n\
      key = 1\n\
      pair = \"two\"   \n\
      val = 1.232e10\n\
    end inner\n\
end test2\n\
    ",
                             "\n\
struct test1\n\
    key1 = \"value\" \n\
    key2 = 1.342    # test comment here \n\
    key3 = 10 \n\
    f = \"none\" \n\
end test1 \n\
\n\
struct outer \n\
    my_key = \"foo\" \n\
    n_key = 1 \n\
\n\
    struct inner   # Another comment \"here\" \n\
      key = 1 \n\
      #pair = \"two\" \n\
      val = 1.232e10 \n\
\n\
      struct test1 \n\
        key = -5 \n\
      end test1 \n\
    end inner \n\
end outer \n\
\n\
struct test1 \n\
    different = 1 \n\
    other = 2 \n\
end test1 \n\
    ",
                             "\n\
struct test1 \n\
    key1 = \"value\" \n\
    key2 = 1.342    # test comment here \n\
    key3 = 10 \n\
    f = \"none\" \n\
end test1 \n\
\n\
struct protos \n\
  proto joint_proto \n\
    key1 = $VAR1 \n\
    key2 = $VAR2 \n\
    not_a_var = 27 \n\
  end joint_proto \n\
end protos \n\
\n\
struct outer \n\
    my_key = \"foo\" \n\
    n_key = 1 \n\
\n\
    struct inner   # Another comment \"here\" \n\
      key = 1 \n\
      #pair = \"two\" \n\
      val = 1.232e10 \n\
\n\
      struct test1 \n\
        key = -5 \n\
      end test1 \n\
    end inner \n\
end outer \n\
\n\
struct ref_in_struct \n\
  key1 = -0.3492 \n\
\n\
  reference protos.joint_proto as hx \n\
    $VAR1 = \"0xdeadbeef\" \n\
    $VAR2 = \"3\" \n\
    +new_var = 5 \n\
    +add_another = -3.14159 \n\
  end hx \n\
end ref_in_struct \n\
\n\
struct test1 \n\
    different = 1 \n\
    other = 2 \n\
end test1 \n\
    ",
                             "\n\n\n\
struct protos \n\
\n\
    proto joint_proto \n\
      leg = $LEG \n\
      dof = $DOF  # This should be $LEG.$DOF, but not supported yet \n\
    end joint_proto \n\
\n\
    proto leg_proto \n\
      key_1 = \"foo\" \n\
      key_2 = \"bar\" \n\
      key_3 = $LEG \n\
\n\
      reference protos.joint_proto as hx  # Can we make the 'hx' here be represented by $DOF? \n\
        $DOF = \"hx\" \n\
        +extra_key = 1.e-3 \n\
      end hx \n\
    end leg_proto \n\
end protos \n\
\n\
struct outer \n\
    my_key = \"foo\" \n\
    n_key = 1 \n\
\n\
    struct inner   # Another comment \"here\" \n\
      key = 1 \n\
      #pair = \"two\" \n\
      val = 1.232e10 \n\
\n\
      struct test1 \n\
        key = -5 \n\
      end test1 \n\
\n\
      key_afer = 3 \n\
    end inner \n\
\n\
    key_between = 4 \n\
\n\
    reference protos.leg_proto as fl \n\
      $LEG = \"fl\" \n\
      $DOF = \"hx\" \n\
    end fl \n\
end outer \n\
\n\
\n\
\n\
"};

  for (const auto &content : config_strs) {
    runTest<config::grammar>(test_num++, content, pdot);
  }
  return 0;
}

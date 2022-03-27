
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/analyze.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>
#include <tao/pegtl/contrib/parse_tree_to_dot.hpp>
#include <tao/pegtl/contrib/trace.hpp>
#include <vector>

#include "config_grammar.h"

namespace peg = TAO_PEGTL_NAMESPACE;

namespace dummy {
struct grammar : peg::star<peg::any> {};

template <typename Rule>
using selector = peg::parse_tree::selector<Rule,
                                           peg::parse_tree::store_content::on<peg::any>>;
}

namespace filename {

template <typename Rule>
struct selector :
    peg::parse_tree::selector<Rule, peg::parse_tree::store_content::on<FILEPART, FILENAME, EXT>,
                              peg::parse_tree::remove_content::on<ALPHAPLUS>> {};

template <>
struct selector<config::INCLUDE> : std::true_type {
  template <typename... States>
  static void transform(std::unique_ptr<peg::parse_tree::node>& n, States&&... st) {
    std::cout << "Successfully found a " << n->type << " node in " << n->source << "!\n";
    std::string filename = "";
    for (const auto& c : n->children) {
      std::cout << "Has child of type " << c->type << ".\n";
      if (c->has_content()) {
        std::cout << "  Content: " << c->string_view() << "\n";
      }
      if (c->is_type<filename::FILENAME>()) {
        filename = c->string();
      }
    }
    std::cout << std::endl;

    std::cout << " --- Node has " << n->children.size() << " children.\n";

    //// Let's try to parse the "include" file and replace this node with the output!
    // Build the parse path:
    auto include_path = std::filesystem::path(n->source).parent_path() / filename;

    std::cout << "Trying to parse: " << include_path << std::endl;
    auto input_file = peg::file_input(include_path);

    // This is a bit of a hack. We need to use a 'string_input' here because if we don't, the
    // 'file_input' will go out of scope, and the generating the 'dot' output will fail. This would
    // likely be the case when trying to walk the parse tree later.
    // TODO: Find a more elegant way of
    // doing this.
    const auto file_contents = std::string(input_file.begin(), input_file.size());
    auto input = peg::string_input(file_contents, input_file.source());

    // TODO: Convert this to a `parse_nested` call, which will likely fix the issues noted above of
    // requiring a string input instead of a file input.
    // See: https://github.com/taocpp/PEGTL/blob/main/doc/Inputs-and-Parsing.md#nested-parsing

    if (auto new_tree = peg::parse_tree::parse<dummy::grammar, dummy::selector>(input)) {
#if PRINT_DOT
      peg::parse_tree::print_dot(std::cout, *new_tree->children.back());
#endif
      auto c = std::move(new_tree->children.back());
      std::cout << " --- Parsed include file has " << c->children.size() << " children.\n";
      new_tree->children.pop_back();
      n->remove_content();
      n = std::move(c);
      std::cout << "c=" << c.get() << std::endl;
      std::cout << "n=" << n.get() << std::endl;
      std::cout << " --- n has " << n->children.size() << " children." << std::endl;
      std::cout << " --- Source: " << n->source << std::endl;
      std::cout << " ------ size: " << n->source.size() << std::endl;
    } else {
      std::cout << "Failed to properly parse " << include_path << "." << std::endl;
    }
  }
};

}  // namespace filename

namespace include_file {
struct grammar : peg::seq<config::TAIL, config::include_list> {};
}  // namespace include_file


#if 0
template <typename GTYPE>
auto runTest(size_t idx, const std::string &test_str, bool pdot = true)
    -> bool {
  std::cout << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n";
  std::cout << "Parsing example " << idx << ":\n";
  std::cout << test_str << std::endl;
  std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";

  peg::memory_input in(test_str, "example " + std::to_string(idx));

  bool ret{false};
  std::string out;
  try {
    if (const auto root = peg::parse_tree::parse<GTYPE, filename::selector>(in)) {
      if (pdot) {
        peg::parse_tree::print_dot(std::cout, *root);
      }
      std::cout << "  Parse tree success.\n";
    } else {
      std::cout << "  Parse tree failure!\n";
      in.restart();
      peg::standard_trace<GTYPE>(in);
    }

    in.restart();
    ret = peg::parse<GTYPE>(in);
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
#endif

template <typename GTYPE, typename SOURCE>
auto runTest(SOURCE& src, bool pdot = true) -> bool {
  std::cout << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n";
  std::cout << "Parsing: " << src.source() << "\n----- content -----\n";
  std::cout << std::string_view(src.begin(), src.size()) << std::endl;
  std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";

  bool ret{false};
  try {
    if (const auto root = peg::parse_tree::parse<GTYPE>(src)) {
      if (pdot) {
        peg::parse_tree::print_dot(std::cout, *root);
      }
      std::cout << "  Parse tree success.\n";
    } else {
      std::cout << "  Parse tree failure!\n";
      src.restart();
      peg::standard_trace<GTYPE>(src);
    }
    // Re-use of an input is highly discouraged, but given that we probably
    // won't call both `parse_tree::parse` and `parse` in the same run, we'll
    // re-use the input here for convenience.
    src.restart();
    ret = peg::parse<GTYPE>(src);
    std::cout << "  Parse " << (ret ? "success" : "failure") << std::endl;
  } catch (const peg::parse_error& e) {
    std::cout << "!!!\n";
    std::cout << "  Parser failure!\n";
    const auto p = e.positions().front();
    std::cout << e.what() << '\n' << src.line_at(p) << '\n' << std::setw(p.column) << '^' << '\n';
    std::cout << "!!!\n";
  }

  std::cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n";
  return ret;
}

template <typename GTYPE>
auto runTest(size_t idx, const std::string& test_str, bool pdot = true) -> bool {
  peg::memory_input in(test_str, "example " + std::to_string(idx));

  return runTest<GTYPE>(in, pdot);
}

auto main() -> int {
  const bool pdot = false;

  if (peg::analyze<filename::grammar>() != 0) {
    std::cout << "Something in the grammar is broken!" << std::endl;
    return 1;
  }

  bool ret{true};
  size_t test_num = 1;

  std::vector<std::string> test_filenames = {"example1.cfg", "../example2.cfg", "foo/bar/baz.cfg"};

  for (const auto& e : test_filenames) {
    ret &= runTest<filename::FILENAME>(test_num++, e);
  }

  runTest<include_file::grammar>(999, "include ../robot/default.cfg\n");

  return (ret ? 0 : 10);
}

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/analyze.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>
#include <tao/pegtl/contrib/parse_tree_to_dot.hpp>
#include <tao/pegtl/contrib/trace.hpp>
#include <vector>

#include "config_actions.h"
#include "config_grammar.h"
#include "config_selector.h"

// TODO(rose@): Turn this into an actual test suite.

namespace peg = TAO_PEGTL_NAMESPACE;

template <typename GTYPE, typename SOURCE>
auto runTest(SOURCE &src, bool pdot = true) -> bool {
  std::cout << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n";
  std::cout << "Parsing: " << src.source() << "\n----- content -----\n";
  std::cout << std::string_view(src.begin(), src.size()) << std::endl;
  std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";

  bool ret{false};
  config::ActionData out;
  try {
    if (const auto root = peg::parse_tree::parse<GTYPE, config::selector>(src)) {
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
    ret = peg::parse<GTYPE, config::action>(src, out);
    std::cout << "  Parse " << (ret ? "success" : "failure") << std::endl;
    std::cout << "output: \n";
    out.print(std::cout);
  } catch (const peg::parse_error &e) {
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
auto runTest(size_t idx, const std::string &test_str, bool pdot = true) -> bool {
  peg::memory_input in(test_str, "example " + std::to_string(idx));

  return runTest<GTYPE>(in, pdot);
}

auto main() -> int {
  const bool pdot = false;
  bool ret{true};

  size_t test_num = 1;

  std::vector config_strs = {
      "\n\
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
"};

  for (const auto &content : config_strs) {
    peg::memory_input in(content, "example " + std::to_string(test_num++));
    ret &= runTest<config::grammar>(in, pdot);
  }

  for (size_t i = 1;; ++i) {
    const auto cfg_file =
        std::filesystem::path(EXAMPLE_DIR) / ("config_example" + std::to_string(i) + ".cfg");
    if (!std::filesystem::exists(cfg_file)) {
      break;
    }

    peg::file_input in(cfg_file);
    ret &= runTest<config::grammar>(in, pdot);
  }

  return (ret ? 0 : 1);
}

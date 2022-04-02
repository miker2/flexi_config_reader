#include <filesystem>
#include <iostream>
//#include <range/v3/all.hpp>  // get everything (consider pruning this down a bit)
#include <range/v3/action/remove_if.hpp>
#include <tao/pegtl.hpp>

#include "config_actions.h"
#include "config_grammar.h"
#include "config_selector.h"

namespace config::post_process {
auto flatten(const types::CfgMap& in, const std::string& base_name, types::CfgMap flattened = {})
    -> types::CfgMap {
  for (const auto& e : in) {
    const auto new_name = utils::join({base_name, e.first}, ".");
    const auto struct_like = dynamic_pointer_cast<types::ConfigStructLike>(e.second);
    if (struct_like != nullptr) {
      flattened = flatten(struct_like->data, new_name, flattened);
    } else {
      flattened[new_name] = e.second;
      std::cout << new_name << " = " << e.second << std::endl;
    }
  }
  return flattened;
}

}  // namespace config::post_process

auto main(int argc, char* argv[]) -> int {
  bool success = true;

  if (argc != 2) {
    std::cerr << "No file specified.\n";
    std::cerr << "usage: " << std::filesystem::path(argv[0]).filename().string() << " CFG_FILE"
              << std::endl;
    return -1;
  }

  peg::file_input cfg_file(argv[1]);
  config::ActionData out;

  try {
    success = peg::parse<config::grammar, config::action>(cfg_file, out);
    // If parsing is successful, all of these containers should be emtpy (consumed into
    // 'out.cfg_res').
    success &= out.keys.empty();
    success &= out.flat_keys.empty();
    success &= out.objects.empty();
    success &= out.obj_res == nullptr;

    // Eliminate any vector elements with an empty map.
    out.cfg_res |=
        ranges::actions::remove_if([](const config::types::CfgMap& m) { return m.empty(); });

    std::cout << "\n";
    std::cout << "  Parse " << (success ? "success" : "failure") << std::endl;
    std::cout << "  cfg_res size: " << out.cfg_res.size() << std::endl;

    std::cout << "\noutput: \n";
    out.print();
  } catch (const peg::parse_error& e) {
    success = false;
    std::cout << "!!!\n";
    std::cout << "  Parser failure!\n";
    const auto p = e.positions().front();
    std::cout << e.what() << '\n'
              << cfg_file.line_at(p) << '\n'
              << std::setw(p.column) << '^' << '\n';
    std::cout << "!!!\n";
  }

  for (const auto& e : out.cfg_res) {
    config::post_process::flatten(e, "");
  }

  return (success ? 0 : 1);
}

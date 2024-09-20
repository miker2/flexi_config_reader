#include <fmt/color.h>
#include <fmt/format.h>

#include <exception>
#include <filesystem>
#include <magic_enum.hpp>
#include <span>

#include "flexi_cfg/logger.h"
#include "flexi_cfg/parser.h"
#include "flexi_cfg/reader.h"

auto main(int argc, char* argv[]) -> int {  // NOLINT(bugprone-exception-escape)
  try {
    std::span<char*> args(argv, argc);
    if (argc < 2) {
      std::cerr << "No file specified.\n";
      std::cerr << "usage: " << std::filesystem::path(args[0]).filename().string() << " CFG_FILE"
                << '\n';
      return -1;
    }
    auto log_level = flexi_cfg::logger::Severity::INFO;
    if (argc == 3) {
      auto out = magic_enum::enum_cast<flexi_cfg::logger::Severity>(args[2]);
      if (out.has_value()) {
        log_level = out.value();
      }
    }

    flexi_cfg::logger::setLevel(log_level);

    auto cfg = flexi_cfg::Parser::parse(std::filesystem::path(args[1]));
    std::cout << '\n';
    cfg.dump();

    return EXIT_SUCCESS;
  } catch (const std::exception& e) {
    fmt::print(fmt::fg(fmt::color::red), "{}\n", e.what());
    return EXIT_FAILURE;
  }
}

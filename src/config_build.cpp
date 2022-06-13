#include <fmt/color.h>
#include <fmt/format.h>

#include <exception>
#include <filesystem>
#include <magic_enum.hpp>
#include <span>

#include "flexi_cfg/config/reader.h"
#include "flexi_cfg/logger.h"

auto main(int argc, char* argv[]) -> int {
  try {
    std::span<char*> args(argv, argc);
    if (argc < 2) {
      std::cerr << "No file specified.\n";
      std::cerr << "usage: " << std::filesystem::path(args[0]).filename().string() << " CFG_FILE"
                << std::endl;
      return -1;
    }
    logger::Severity log_level = logger::Severity::INFO;
    if (argc == 3) {
      auto out = magic_enum::enum_cast<logger::Severity>(args[2]);
      if (out.has_value()) {
        log_level = out.value();
      }
    }
    logger::setLevel(log_level);

    ConfigReader cfg;
    const auto success = cfg.parse(std::filesystem::path(args[1]));
    if (success) {
      std::cout << std::endl;
      cfg.dump();
    }

    return (success ? EXIT_SUCCESS : EXIT_FAILURE);
  } catch (const std::exception& e) {
    fmt::print(fmt::fg(fmt::color::red), "{}\n", e.what());
    return EXIT_FAILURE;
  }
}

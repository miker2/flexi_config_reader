#include <filesystem>
#include <magic_enum.hpp>

#include "config_reader.h"
#include "logger.h"

auto main(int argc, char* argv[]) -> int {
  if (argc < 2) {
    std::cerr << "No file specified.\n";
    std::cerr << "usage: " << std::filesystem::path(argv[0]).filename().string() << " CFG_FILE"
              << std::endl;
    return -1;
  }
  logger::Severity log_level = logger::Severity::INFO;
  if (argc == 3) {
    auto out = magic_enum::enum_cast<logger::Severity>(argv[2]);
    if (out.has_value()) {
      log_level = out.value();
    }
  }
  logger::setLevel(log_level);

  ConfigReader cfg;
  const auto success = cfg.parse(std::filesystem::path(argv[1]));
  if (success) {
    std::cout << "\n" << std::string(35, '!') << " Result " << std::string(35, '!') << std::endl;
    cfg.dump();
  }

  return (success ? 0 : 1);
}

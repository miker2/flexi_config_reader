
#include "config_reader.h"
#include "logger.h"

auto main(int argc, char* argv[]) -> int {
  if (argc != 2) {
    std::cerr << "No file specified.\n";
    std::cerr << "usage: " << std::filesystem::path(argv[0]).filename().string() << " CFG_FILE"
              << std::endl;
    return -1;
  }

  logger::setLevel(logger::Severity::INFO);

  ConfigReader cfg;
  const auto success = cfg.parse(argv[1]);
  if (success) {
    std::cout << "\n" << std::string(35, '!') << " Result " << std::string(35, '!') << std::endl;
    cfg.dump();
  }

  return (success ? 0 : 1);
}


#include "config_reader.h"

auto main(int argc, char* argv[]) -> int {
  if (argc != 2) {
    std::cerr << "No file specified.\n";
    std::cerr << "usage: " << std::filesystem::path(argv[0]).filename().string() << " CFG_FILE"
              << std::endl;
    return -1;
  }

  ConfigReader cfg;
  const auto success = cfg.parse(argv[1]);

  return (success ? 0 : 1);
}

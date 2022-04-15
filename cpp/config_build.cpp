
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

  // This is a hack for now. Just to test that the cfg reading is working.
  {
    const std::string int_key = "outer.inner.key";
    try {
      const auto out = cfg.getValue<int>(int_key);
      std::cout << "Value of '" << int_key << "' is: " << out << std::endl;
    } catch (std::exception& e) {
      std::cout << "getValue failure!!!\n" << e.what() << std::endl;
    }
  }
  {
    const std::string float_key = "outer.inner.val";
    try {
      const auto out = cfg.getValue<float>(float_key);
      std::cout << "Value of '" << float_key << "' is: " << out << std::endl;
    } catch (std::exception& e) {
      std::cout << "getValue failure!!!\n" << e.what() << std::endl;
    }
  }
  {
    const std::string vec_key = "outer.inner.test1.key";
    try {
      const auto var = cfg.getValue<std::vector<float>>(vec_key);
      std::cout << "Value of '" << vec_key << "' is: " << var << std::endl;
    } catch (std::exception& e) {
      std::cout << "getValue failure!!!\n" << e.what() << std::endl;
    }
  }
  {
    const std::string int_key = "a_top_level_flat_key";
    try {
      const auto out = cfg.getValue<int>(int_key);
      std::cout << "Value of '" << int_key << "' is: " << out << std::endl;
    } catch (std::exception& e) {
      std::cout << "getValue failure!!!\n" << e.what() << std::endl;
    }
  }
  {
    const std::string float_key = "outer.inner.val";
    try {
      float out{};
      cfg.getValue(float_key, out);
      std::cout << "Value of '" << float_key << "' is: " << out << std::endl;
    } catch (std::exception& e) {
      std::cout << "getValue failure!!!\n" << e.what() << std::endl;
    }
  }
  {
    const std::string vec_key = "outer.inner.test1.key";
    try {
      std::vector<float> var;
      cfg.getValue(vec_key, var);
      std::cout << "Value of '" << vec_key << "' is: " << var << std::endl;
    } catch (std::exception& e) {
      std::cout << "getValue failure!!!\n" << e.what() << std::endl;
    }
  }
  {
    const std::string vec_key = "outer.inner.test1.key";
    try {
      std::array<double, 3> var;
      cfg.getValue(vec_key, var);
      std::cout << "Value of '" << vec_key << "' is: " << var << std::endl;
    } catch (std::exception& e) {
      std::cout << "getValue failure!!!\n" << e.what() << std::endl;
    }
  }

  return (success ? 0 : 1);
}

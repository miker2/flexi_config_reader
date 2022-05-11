// Read a configuration file and get values from it.

#include <fmt/color.h>
#include <fmt/format.h>

#include <array>
#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "config_reader.h"
#include "logger.h"

auto main(int argc, char* argv[]) -> int {
  try {
    const auto cfg_file = std::filesystem::path(EXAMPLE_DIR) / "config_example5.cfg";

    ConfigReader cfg;
    if (!cfg.parse(cfg_file)) {
      logger::error("Failed to parse {}", cfg_file.string());
    }

    logger::setLevel(logger::Severity::INFO);

    std::cout << std::endl;
    // Read a variety of values from the config file (and print their values).
    {
      const std::string int_key = "outer.inner.key";
      const auto out = cfg.getValue<int>(int_key);
      fmt::print("Value of '{}' is: {}\n", int_key, out);
    }
    {
      // getValue to return a value, or it can take a reference to a value:
      const std::string float_key = "outer.fl.hx.extra_key";
      auto out = cfg.getValue<float>(float_key);
      fmt::print("Value of '{}' is: {}\n", float_key, out);
      // This is the same as the above
      cfg.getValue(float_key, out);
      fmt::print("Value of '{}' is: {}\n", float_key, out);

      // Similarly, this value can be read as a float or a double. It doesn't really matter.
      auto out_d = cfg.getValue<double>(float_key);
      fmt::print("Value of '{}' as a double is: {}\n", float_key, out_d);

      // It however, cannot be read as an integer type!
      try {
        auto out_i = cfg.getValue<int>(float_key);
        fmt::print("Value of '{}' as an int is: {}\n", float_key, out_i);
      } catch (config::MismatchTypeException& e) {
        logger::error("!!! getValue failure !!!\n{}", e.what());
      }
    }
    {
      // Vectors of values can also be read.
      const std::string vec_key = "outer.inner.test1.key";
      auto var = cfg.getValue<std::vector<float>>(vec_key);
      fmt::print("Value of '{}' is: {}\n", vec_key, var);
      // Just as with scalar values, passing by reference works for vectors.
      cfg.getValue(vec_key, var);
      fmt::print("Value of '{}' is: {}\n", vec_key, var);

      // This same entry can be read using a std::array, which provides length checking.
      // E.g. This will pass:
      auto arr_var = cfg.getValue<std::array<float, 3>>(vec_key);
      fmt::print("Value of array '{}' is: {}\n", vec_key, arr_var);
      // But this will fail:
      try {
        // There are only 3 values contained by the key, but we're looking for 4!
        auto arr_var = cfg.getValue<std::array<float, 4>>(vec_key);
        logger::info("Value of {} is {}", vec_key, arr_var);
      } catch (std::exception& e) {
        logger::error("!!! getValue failure !!!\n{}", e.what());
      }
    }
    {
      const std::string int_key = "a_top_level_flat_key";
      const auto out = cfg.getValue<int>(int_key);
      fmt::print("Value of '{}' is: {}\n", int_key, out);

      // TODO: Decide if we should allow this or if it should be a failure.
      const auto out_f = cfg.getValue<float>(int_key);
      fmt::print("Value of '{}' is: {}\n", int_key, out_f);
    }
  } catch (const std::exception& e) {
    fmt::print(fmt::fg(fmt::color::red), "{}\n", e.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

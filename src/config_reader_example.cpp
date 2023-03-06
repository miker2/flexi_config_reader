// Read a configuration file and get values from it.

#include <fmt/color.h>
#include <fmt/format.h>

#include <array>
#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "flexi_cfg/logger.h"
#include "flexi_cfg/parser.h"
#include "flexi_cfg/reader.h"
#include "flexi_cfg/utils.h"

auto main(int argc, char* argv[]) -> int {
  try {
    const auto cfg_file = std::filesystem::path(EXAMPLE_DIR) / "config_example5.cfg";

    auto cfg = flexi_cfg::Parser::parse(cfg_file);

    flexi_cfg::logger::setLevel(flexi_cfg::logger::Severity::INFO);

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
      } catch (flexi_cfg::config::MismatchTypeException& e) {
        flexi_cfg::logger::error("!!! getValue failure !!!\n{}", e.what());
      }
    }
    {
      // Vectors of values can also be read.
      const std::string vec_key = "outer.inner.test1.key";
      auto var = cfg.getValue<std::vector<float>>(vec_key);
      fmt::print("Value of '{}' is: {}\n", vec_key, var);
      var.clear();
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
        flexi_cfg::logger::info("Value of {} is {}", vec_key, arr_var);
      } catch (std::exception& e) {
        flexi_cfg::logger::error("!!! getValue failure !!!\n{}", e.what());
      }
    }
    {
      const auto my_str_key = flexi_cfg::utils::join({"outer", "inner", "key_w_str"}, ".");
      const auto my_strs = cfg.getValue<std::vector<std::string>>(my_str_key);
      fmt::print("{}: [{}]\n", my_str_key, fmt::join(my_strs, ", "));
    }
    {
      const std::string int_key = "a_top_level_flat_key";
      const auto out = cfg.getValue<int>(int_key);
      fmt::print("Value of '{}' is: {}\n", int_key, out);

      // TODO(miker2): Decide if we should allow this or if it should be a failure.
      const auto out_f = cfg.getValue<float>(int_key);
      fmt::print("Value of '{}' is: {}\n", int_key, out_f);
    }
    /*
    {
      // Read a list of lists
      const auto my_nested_list_key = "outer.multi_list";
      const auto out = cfg.getValue<std::vector<std::vector<int>>>(my_nested_list_key);
      fmt::print("Value of '{}' is : [{}]", my_nested_list_key, fmt::join(out, ", "));
    }
    */
    {
      // We can get a sub-struct:
      const std::string struct_key = "this.is";
      const auto out = cfg.getValue<flexi_cfg::Reader>(struct_key);
      fmt::print("Value of '{}' is: \n", struct_key);
      out.dump();

      try {
        // Look for a sub-struct that isn't a substruct
        const std::string bad_key = "this.is.a.flat.key";
        const auto fail = cfg.getValue<flexi_cfg::Reader>(bad_key);
      } catch (std::exception& e) {
        flexi_cfg::logger::error("!!! getValue failure !!!\n{}", e.what());
      }

      try {
        // Look for a sub-struct that isn't a substruct
        const std::string bad_key = "this.is.an";
        const auto fail = cfg.getValue<flexi_cfg::Reader>(bad_key);
      } catch (std::exception& e) {
        flexi_cfg::logger::error("!!! getValue failure !!!\n{}", e.what());
      }
    }
    {
      // Find the name of all structs containing a specific key
      const std::string key = "offset";
      const auto& structs = cfg.findStructsWithKey(key);
      fmt::print("Found the following that contain '{}': \n\t{}\n", key,
                 fmt::join(structs, "\n\t"));
    }
  } catch (const std::exception& e) {
    fmt::print(fmt::fg(fmt::color::red), "{}\n", e.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

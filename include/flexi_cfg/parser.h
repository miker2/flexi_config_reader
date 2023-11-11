#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "flexi_cfg/config/actions.h"
#include "flexi_cfg/config/classes.h"
#include "flexi_cfg/reader.h"

namespace flexi_cfg {

class Parser {
 public:
  static auto parse(const std::filesystem::path& cfg_filename, 
                    std::optional<std::filesystem::path> root_dir = std::nullopt) -> Reader;

  static auto parseFromString(std::string_view cfg_string, std::string_view source = "unknown") -> Reader;

 private:
  Parser() = default;

  auto resolveConfig(config::ActionData& state) -> const config::types::CfgMap&;

  auto flattenAndFindProtos(const config::types::CfgMap& in, const std::string& base_name,
                            config::types::CfgMap flattened = {}) -> config::types::CfgMap;

  /// \brief Remove the protos from merged dictionary
  /// \param[in/out] cfg_map - The top level (resolved) config map
  void stripProtos(config::types::CfgMap& cfg_map) const;

  /// \brief Walk through CfgMap and find all references. Convert them to structs
  /// \param[in/out] cfg_map
  /// \param[in] base_name - starting point for resolving references
  /// \param[in] ref_vars - map of all reference variables available in the current context
  /// \param[in] refd_protos - vector of all protos already referenced. Used to track cycles.
  void resolveReferences(config::types::CfgMap& cfg_map, const std::string& base_name,
                         const config::types::RefMap& ref_vars = {},
                         const std::vector<std::string>& refd_protos = {}) const;

  config::types::ProtoMap protos_{};

  config::types::CfgMap cfg_data_;
};

inline auto parse(const std::filesystem::path& cfg_filename) -> Reader {
  return Parser::parse(cfg_filename);
}

inline auto parse(std::string_view cfg_string, std::string_view source = "unknown") -> Reader {
  return Parser::parse(cfg_string, source);
}

}  // namespace flexi_cfg

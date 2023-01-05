#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "flexi_cfg/config/actions.h"
#include "flexi_cfg/config/classes.h"

namespace flexi_cfg {

class Parser {
 public:
  Parser() = default;
  ~Parser() = default;

    auto parse(const std::filesystem::path& cfg_filename) -> bool;  // PARSER

  auto parse(std::string_view cfg_string, std::string_view source = "unknown") -> bool;  // PARSER

 private:
  void resolveConfig(config::ActionData& state);  // PARSER

  auto flattenAndFindProtos(const config::types::CfgMap& in, const std::string& base_name,
                            config::types::CfgMap flattened = {}) -> config::types::CfgMap; // PARSER

  /// \brief Remove the protos from merged dictionary
  /// \param[in/out] cfg_map - The top level (resolved) config map
  void stripProtos(config::types::CfgMap& cfg_map) const;   // PARSER

  /// \brief Walk through CfgMap and find all references. Convert them to structs
  /// \param[in/out] cfg_map
  /// \param[in] base_name - starting point for resolving references
  /// \param[in] ref_vars - map of all reference variables available in the current context
  /// \param[in] refd_protos - vector of all protos already referenced. Used to track cycles.
  void resolveReferences(config::types::CfgMap& cfg_map, const std::string& base_name,
                         const config::types::RefMap& ref_vars = {},
                         const std::vector<std::string>& refd_protos = {}) const;  // PARSER

  config::types::ProtoMap protos_{};

  config::types::CfgMap cfg_data_;
};

}

#include <gtest/gtest.h>

#include <filesystem>
#include <iostream>
#include <string>
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>

#include "cfg_reader/config_actions.h"
#include "cfg_reader/config_grammar.h"
#include "cfg_reader/config_reader.h"
#include "cfg_reader/config_selector.h"
#include "cfg_reader/logger.h"

namespace peg = TAO_PEGTL_NAMESPACE;

class InputString : public testing::TestWithParam<std::string> {};

// NOLINTNEXTLINE
TEST_P(InputString, ParseTree) {
  auto parse = []() {
    peg::memory_input in(GetParam(), "From content");
    auto root = peg::parse_tree::parse<config::grammar, config::selector>(in);
  };
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-goto)
  EXPECT_NO_THROW(parse());
}

// NOLINTNEXTLINE
TEST_P(InputString, Parse) {
  logger::setLevel(logger::Severity::INFO);
  auto parse = []() {
    peg::memory_input in(GetParam(), "From content");
    config::ActionData out;
    return peg::parse<config::grammar, config::action>(in, out);
  };
  bool ret{false};
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-goto)
  EXPECT_NO_THROW(ret = parse());
  EXPECT_TRUE(ret);
}

// NOLINTNEXTLINE
TEST_P(InputString, ConfigReaderParse) {
  logger::setLevel(logger::Severity::INFO);
  ConfigReader cfg;
  bool success{false};
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-goto)
  EXPECT_NO_THROW(success = cfg.parse(GetParam(), "From String"));
  EXPECT_TRUE(success);
}

// NOLINTNEXTLINE
INSTANTIATE_TEST_SUITE_P(ConfigParse, InputString, testing::Values(std::string("\n\
struct test1 {\n\
    key1 = \"value\"\n\
    key2 = 1.342    # test comment here\n\
    key3 = 10\n\
    f = \"none\"\n\
}\n\
\n\
struct test2 {\n\
    my_key = \"foo\"  \n\
    n_key = 1\n\
}\n\
")));

/// File-based input
class FileInput : public testing::TestWithParam<std::filesystem::path> {};

namespace {
auto baseDir() -> const std::filesystem::path& {
  static const std::filesystem::path base_dir = std::filesystem::path(EXAMPLE_DIR);
  return base_dir;
}

auto filenameGenerator() -> std::vector<std::filesystem::path> {
  std::vector<std::filesystem::path> files;
  auto genFilename = [](std::size_t idx) -> std::filesystem::path {
    return baseDir() / ("config_example" + std::to_string(idx) + ".cfg");
  };
  for (size_t idx = 1;; ++idx) {
    const auto cfg_file = genFilename(idx);
    if (!std::filesystem::exists(cfg_file)) {
      break;
    }
    files.emplace_back(cfg_file.filename());
  }
  return files;
}
}  // namespace

// NOLINTNEXTLINE
TEST_P(FileInput, ParseTree) {
  auto parse = []() {
    peg::file_input in(baseDir() / GetParam());
    auto root = peg::parse_tree::parse<config::grammar, config::selector>(in);
  };
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-goto)
  EXPECT_NO_THROW(parse());
}

// NOLINTNEXTLINE
TEST_P(FileInput, Parse) {
  logger::setLevel(logger::Severity::WARN);
  auto parse = []() {
    peg::file_input in(baseDir() / GetParam());
    config::ActionData out;
    return peg::parse<config::grammar, config::action>(in, out);
  };
  bool ret{false};
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-goto)
  EXPECT_NO_THROW(ret = parse());
  EXPECT_TRUE(ret);
}

// NOLINTNEXTLINE
TEST_P(FileInput, ConfigReaderParse) {
  logger::setLevel(logger::Severity::WARN);
  ConfigReader cfg;
  bool success{false};
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-goto)
  EXPECT_NO_THROW(success = cfg.parse(baseDir() / GetParam()));
  EXPECT_TRUE(success);
}

// NOLINTNEXTLINE
INSTANTIATE_TEST_SUITE_P(ConfigParse, FileInput, testing::ValuesIn(filenameGenerator()));

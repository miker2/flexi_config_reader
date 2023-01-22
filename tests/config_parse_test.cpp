#include <gtest/gtest.h>

#include <filesystem>
#include <iostream>
#include <string>
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>

#include "flexi_cfg/config/actions.h"
#include "flexi_cfg/config/grammar.h"
#include "flexi_cfg/config/selector.h"
#include "flexi_cfg/logger.h"
#include "flexi_cfg/parser.h"
#include "flexi_cfg/reader.h"

namespace peg = TAO_PEGTL_NAMESPACE;

class InputString : public testing::TestWithParam<std::string> {};

TEST_P(InputString, ParseTree) {
  auto parse = []() {
    peg::memory_input in(GetParam(), "From content");
    auto root = peg::parse_tree::parse<flexi_cfg::config::grammar, flexi_cfg::config::selector>(in);
  };
  EXPECT_NO_THROW(parse());
}

TEST_P(InputString, Parse) {
  flexi_cfg::logger::setLevel(flexi_cfg::logger::Severity::INFO);
  auto parse = []() {
    peg::memory_input in(GetParam(), "From content");
    flexi_cfg::config::ActionData out;
    return peg::parse<flexi_cfg::config::grammar, flexi_cfg::config::action>(in, out);
  };
  bool ret{false};
  EXPECT_NO_THROW(ret = parse());
  EXPECT_TRUE(ret);
}

TEST_P(InputString, ConfigReaderParse) {
  flexi_cfg::logger::setLevel(flexi_cfg::logger::Severity::INFO);
  flexi_cfg::Reader cfg({}, "");  // Nominally, we wouldn't do this, but we need a mechanism to
                                  // capture the output of 'parse' from within the "try/catch" block

  EXPECT_NO_THROW(cfg = flexi_cfg::Parser::parse(GetParam(), "From String"));
  EXPECT_TRUE(cfg.exists("test1.key1"));
  EXPECT_EQ(cfg.getValue<std::string>("test1.key1"), "value");
  EXPECT_TRUE(cfg.exists("test1.key2"));
  EXPECT_FLOAT_EQ(cfg.getValue<float>("test1.key2"), 1.342F);
  EXPECT_TRUE(cfg.exists("test1.key3"));
  EXPECT_EQ(cfg.getValue<int>("test1.key3"), 10);
  EXPECT_TRUE(cfg.exists("test1.f"));
  EXPECT_EQ(cfg.getValue<std::string>("test1.f"), "none");
  EXPECT_TRUE(cfg.exists("test2.my_key"));
  EXPECT_EQ(cfg.getValue<std::string>("test2.my_key"), "foo");
  EXPECT_TRUE(cfg.exists("test2.n_key"));
  EXPECT_EQ(cfg.getValue<bool>("test2.n_key"), true);
}

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
    n_key = true\n\
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

TEST_P(FileInput, ParseTree) {
  auto parse = []() {
    peg::file_input in(baseDir() / GetParam());
    auto root = peg::parse_tree::parse<flexi_cfg::config::grammar, flexi_cfg::config::selector>(in);
  };
  EXPECT_NO_THROW(parse());
}

TEST_P(FileInput, Parse) {
  flexi_cfg::logger::setLevel(flexi_cfg::logger::Severity::WARN);
  auto parse = []() {
    peg::file_input in(baseDir() / GetParam());
    flexi_cfg::config::ActionData out;
    out.base_dir = baseDir();
    return peg::parse<flexi_cfg::config::grammar, flexi_cfg::config::action>(in, out);
  };
  bool ret{false};
  EXPECT_NO_THROW(ret = parse());
  EXPECT_TRUE(ret);
}

TEST_P(FileInput, ConfigReaderParse) {
  flexi_cfg::logger::setLevel(flexi_cfg::logger::Severity::WARN);
  EXPECT_NO_THROW(flexi_cfg::Parser::parse(baseDir() / GetParam()));
}

INSTANTIATE_TEST_SUITE_P(ConfigParse, FileInput, testing::ValuesIn(filenameGenerator()));

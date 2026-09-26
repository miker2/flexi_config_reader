#include "flexi_cfg/render.h"

#include <fmt/format.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "flexi_cfg/config/classes.h"
#include "flexi_cfg/logger.h"
#include "flexi_cfg/parser.h"
#include "flexi_cfg/reader.h"

namespace {

// Reader::getCfgMap() is protected; a derived type re-exposes it so these tests can render a
// single node rather than the whole tree.
struct Probe : flexi_cfg::Reader {
  explicit Probe(const flexi_cfg::Reader& reader) : flexi_cfg::Reader(reader) {}
  using flexi_cfg::Reader::getCfgMap;
};

constexpr std::string_view NESTED_CFG = R"(top = 4

struct outer {
  a = 1

  struct middle {
    b = 2

    struct inner {
      c = 3
    }
  }
}
)";

auto splitLines(const std::string& text) -> std::vector<std::string> {
  std::vector<std::string> lines;
  std::istringstream stream(text);
  std::string line;
  while (std::getline(stream, line)) {
    lines.push_back(line);
  }
  return lines;
}

auto dumpToString(const flexi_cfg::Reader& cfg, const flexi_cfg::DumpOptions& opts) -> std::string {
  std::stringstream ss;
  cfg.dump(ss, opts);
  return ss.str();
}

auto nestedCfg() -> flexi_cfg::Reader {
  flexi_cfg::logger::setLevel(flexi_cfg::logger::Severity::CRITICAL);
  return flexi_cfg::Parser::parseFromString(NESTED_CFG, "nested.cfg");
}

TEST(RenderTest, ShowSourceOn) {
  const auto cfg_ = nestedCfg();
  const auto out = dumpToString(cfg_, flexi_cfg::DumpOptions{.show_source = true});
  EXPECT_THAT(out, testing::HasSubstr("top = 4  # nested.cfg:1\n"));
  EXPECT_THAT(out, testing::HasSubstr("a = 1  # nested.cfg:4\n"));
  EXPECT_THAT(out, testing::HasSubstr("c = 3  # nested.cfg:10\n"));
}

TEST(RenderTest, ShowSourceOff) {
  const auto cfg_ = nestedCfg();
  const auto out = dumpToString(cfg_, flexi_cfg::DumpOptions{.show_source = false});
  EXPECT_THAT(out, testing::HasSubstr("top = 4\n"));
  EXPECT_THAT(out, testing::HasSubstr("c = 3\n"));
  // No location comment survives anywhere, and neither does the origin chain that rides on it.
  EXPECT_THAT(out, testing::Not(testing::HasSubstr("#")));
  EXPECT_THAT(out, testing::Not(testing::HasSubstr("nested.cfg")));
}

TEST(RenderTest, ShowSourceOffDropsOriginChain) {
  const auto cfg_ = nestedCfg();
  // A value reached through a reference carries an origin chain, which only the source comment
  // renders. Turning the comment off has to take the whole chain with it.
  constexpr std::string_view cfg_str = R"(proto p {
  a = 0
}

reference p as foo {
}
)";
  const auto cfg = flexi_cfg::Parser::parseFromString(cfg_str, "chain.cfg");
  EXPECT_THAT(dumpToString(cfg, flexi_cfg::DumpOptions{.show_source = true}),
              testing::HasSubstr("a = 0  # chain.cfg:2 (from chain.cfg:1 <- chain.cfg:5)\n"));
  EXPECT_THAT(dumpToString(cfg, flexi_cfg::DumpOptions{.show_source = false}),
              testing::Not(testing::HasSubstr("(from")));
}

TEST(RenderTest, BaseIndentZeroStartsAtColumnZero) {
  const auto cfg_ = nestedCfg();
  const auto out = dumpToString(cfg_, flexi_cfg::DumpOptions{.base_indent = 0});
  const auto lines = splitLines(out);
  ASSERT_FALSE(lines.empty());
  EXPECT_EQ(lines.front(), "top = 4  # nested.cfg:1");
  EXPECT_THAT(out, testing::HasSubstr("\nstruct outer {\n"));
  // One level in is one indent width.
  EXPECT_THAT(out, testing::HasSubstr("\n    a = 1"));
}

TEST(RenderTest, BaseIndentShiftsWholeTree) {
  const auto cfg_ = nestedCfg();
  constexpr std::size_t kBase = 7;
  const auto flush = splitLines(dumpToString(cfg_, flexi_cfg::DumpOptions{.base_indent = 0}));
  const auto shifted = splitLines(dumpToString(cfg_, flexi_cfg::DumpOptions{.base_indent = kBase}));

  ASSERT_EQ(flush.size(), shifted.size());
  const std::string pad(kBase * flexi_cfg::config::types::tw, ' ');
  for (std::size_t i = 0; i < flush.size(); ++i) {
    EXPECT_EQ(shifted[i], pad + flush[i]) << "line " << i;
  }
}

TEST(RenderTest, DefaultOptionsMatchTheBareDumpOverload) {
  const auto cfg_ = nestedCfg();
  // dump(os) has to keep behaving exactly like dump(os, DumpOptions{}).
  std::stringstream bare;
  cfg_.dump(bare);
  EXPECT_EQ(bare.str(), dumpToString(cfg_, flexi_cfg::DumpOptions{}));
}

TEST(RenderTest, NestedSubConfigRendersAtTheCallersIndent) {
  const auto cfg_ = nestedCfg();
  // The node sits two levels deep in the tree it was parsed from. That must have no say in the
  // indentation it renders at: the caller supplies the base indent.
  const Probe probe(cfg_);
  const auto& cfg_map = probe.getCfgMap();
  const auto& outer =
      dynamic_pointer_cast<flexi_cfg::config::types::ConfigStructLike>(cfg_map.at("outer"));
  ASSERT_NE(outer, nullptr);
  const auto& middle =
      dynamic_pointer_cast<flexi_cfg::config::types::ConfigStructLike>(outer->data.at("middle"));
  ASSERT_NE(middle, nullptr);
  const flexi_cfg::config::types::BasePtr node = middle;

  const auto standalone =
      fmt::format("{}", flexi_cfg::Dump{node, {.show_source = false, .base_indent = 0}});
  EXPECT_EQ(standalone, R"(struct middle {
    b = 2
    struct inner {
        c = 3
    }
})");

  constexpr std::size_t kBase = 3;
  const auto embedded =
      fmt::format("{}", flexi_cfg::Dump{node, {.show_source = false, .base_indent = kBase}});
  const std::string pad(kBase * flexi_cfg::config::types::tw, ' ');
  const auto flush_lines = splitLines(standalone);
  const auto embedded_lines = splitLines(embedded);
  ASSERT_EQ(flush_lines.size(), embedded_lines.size());
  for (std::size_t i = 0; i < flush_lines.size(); ++i) {
    EXPECT_EQ(embedded_lines[i], pad + flush_lines[i]) << "line " << i;
  }
}

TEST(RenderTest, OstreamAndFmtPathsAgree) {
  const auto cfg_ = nestedCfg();
  const Probe probe(cfg_);
  const auto& cfg_map = probe.getCfgMap();

  for (const auto& opts : {flexi_cfg::DumpOptions{}, flexi_cfg::DumpOptions{.show_source = false},
                           flexi_cfg::DumpOptions{.show_source = true, .base_indent = 7},
                           flexi_cfg::DumpOptions{.show_source = false, .base_indent = 2}}) {
    std::ostringstream oss;
    oss << flexi_cfg::Dump{cfg_map, opts};
    EXPECT_EQ(oss.str(), fmt::format("{}", flexi_cfg::Dump{cfg_map, opts}))
        << "show_source=" << opts.show_source << " base_indent=" << opts.base_indent;
  }
}

TEST(RenderTest, LegacyShimsAgreeWithTheRenderer) {
  const auto cfg_ = nestedCfg();
  // The bare operator<< and fmt::formatter for a BasePtr are shims over render with default
  // options; they must keep producing what they always did.
  const Probe probe(cfg_);
  const auto& cfg_map = probe.getCfgMap();
  const auto& node = cfg_map.at("outer");

  std::ostringstream oss;
  oss << node;
  EXPECT_EQ(oss.str(), fmt::format("{}", node));
  EXPECT_EQ(oss.str(), fmt::format("{}", flexi_cfg::Dump{node, flexi_cfg::DumpOptions{}}));
}

TEST(RenderTest, NullNodeRendersAsNull) {
  const flexi_cfg::config::types::BasePtr null_node;
  std::ostringstream oss;
  oss << null_node;
  EXPECT_EQ(oss.str(), "NULL");
  EXPECT_EQ(fmt::format("{}", null_node), "NULL");
}

}  // namespace

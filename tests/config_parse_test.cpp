#include <gtest/gtest.h>

#include <atomic>
#include <filesystem>
#include <latch>
#include <string>
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>
#include <thread>

#include "flexi_cfg/config/actions.h"
#include "flexi_cfg/config/grammar.h"
#include "flexi_cfg/config/parser-internal.h"
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
  setLevel(flexi_cfg::logger::Severity::INFO);
  auto parse = []() {
    peg::memory_input in(GetParam(), "From content");
    flexi_cfg::config::ActionData out;
    return flexi_cfg::config::internal::parseCore<flexi_cfg::config::grammar,
                                                  flexi_cfg::config::action>(in, out);
  };
  bool ret{false};
  EXPECT_NO_THROW(ret = parse());
  EXPECT_TRUE(ret);
}

TEST_P(InputString, Reader) {
  setLevel(flexi_cfg::logger::Severity::INFO);
  flexi_cfg::Reader cfg({}, "");  // Nominally, we wouldn't do this, but we need a mechanism to
                                  // capture the output of 'parse' from within the "try/catch" block

  EXPECT_NO_THROW(cfg = flexi_cfg::Parser::parseFromString(GetParam(), "From String"));
  EXPECT_TRUE(cfg.exists("test1.key1"));
  EXPECT_EQ(cfg.getValue<std::string>("test1.key1"), "value");
  EXPECT_EQ(cfg.getType("test1.key1"), flexi_cfg::config::types::Type::kString);
  EXPECT_TRUE(cfg.exists("test1.key2"));
  EXPECT_FLOAT_EQ(cfg.getValue<float>("test1.key2"), 1.342F);
  EXPECT_EQ(cfg.getType("test1.key2"), flexi_cfg::config::types::Type::kNumber);
  EXPECT_TRUE(cfg.exists("test1.key3"));
  EXPECT_EQ(cfg.getValue<int>("test1.key3"), 10);
  EXPECT_EQ(cfg.getType("test1.key3"), flexi_cfg::config::types::Type::kNumber);
  EXPECT_TRUE(cfg.exists("test1.f"));
  EXPECT_EQ(cfg.getValue<std::string>("test1.f"), "none");
  EXPECT_EQ(cfg.getType("test1.f"), flexi_cfg::config::types::Type::kString);
  EXPECT_TRUE(cfg.exists("test2.my_key"));
  EXPECT_EQ(cfg.getValue<std::string>("test2.my_key"), "foo");
  EXPECT_EQ(cfg.getType("test2.my_key"), flexi_cfg::config::types::Type::kString);
  EXPECT_TRUE(cfg.exists("test2.n_key"));
  EXPECT_EQ(cfg.getValue<bool>("test2.n_key"), true);
  EXPECT_EQ(cfg.getType("test2.n_key"), flexi_cfg::config::types::Type::kBoolean);
  EXPECT_TRUE(cfg.exists("test2.inner.list"));
  EXPECT_EQ(cfg.getType("test2.inner.list"), flexi_cfg::config::types::Type::kList);
  EXPECT_EQ(cfg.getValue<std::vector<int>>("test2.inner.list"), std::vector({1, 2, 3, 4}));
  EXPECT_TRUE(cfg.exists("test2.inner.emptyList"));
  EXPECT_EQ(cfg.getType("test2.inner.emptyList"), flexi_cfg::config::types::Type::kList);
  EXPECT_EQ(cfg.getValue<std::vector<int>>("test2.inner.emptyList"), std::vector<int>({}));
  EXPECT_TRUE(cfg.exists("test2.inner.listWithComment"));
  EXPECT_EQ(cfg.getType("test2.inner.listWithComment"), flexi_cfg::config::types::Type::kList);
  EXPECT_EQ(cfg.getValue<std::vector<int>>("test2.inner.listWithComment"), std::vector({0, 2}));
  EXPECT_TRUE(cfg.exists("test2.inner.listWithTrailingComment"));
  EXPECT_EQ(cfg.getType("test2.inner.listWithTrailingComment"),
            flexi_cfg::config::types::Type::kList);
  EXPECT_EQ(cfg.getValue<std::vector<int>>("test2.inner.listWithTrailingComment"),
            std::vector({0, 2}));
  EXPECT_TRUE(cfg.exists("test2"));
  EXPECT_EQ(cfg.getType("test2"), flexi_cfg::config::types::Type::kStruct);
  EXPECT_TRUE(cfg.exists("test1"));
  EXPECT_EQ(cfg.getType("test1"), flexi_cfg::config::types::Type::kStruct);
  EXPECT_TRUE(cfg.exists("test2.inner"));
  EXPECT_EQ(cfg.getType("test2.inner"), flexi_cfg::config::types::Type::kStruct);

  // Coverage for override. The values below should match the value on the following line:
  //  a [override] = 2
  // All of the following variables should match the override value:
  for (const auto& key : {"a", "c", "d", "q.e", "g"}) {
    EXPECT_TRUE(cfg.exists(key));
    EXPECT_EQ(cfg.getValue<float>(key), 2);
    EXPECT_EQ(cfg.getType(key), flexi_cfg::config::types::Type::kNumber);
  }
  //  b [override] = 4
  // All of the following variables should match the override value:
  for (const auto& key : {"b", "e", "f"}) {
    EXPECT_TRUE(cfg.exists(key));
    EXPECT_EQ(cfg.getValue<float>(key), 4);
    EXPECT_EQ(cfg.getType(key), flexi_cfg::config::types::Type::kNumber);
  }
}

// Ensure we don't get optimized out and force "consuming" the value
#pragma GCC push_options
#pragma GCC optimize("-O0")
#pragma clang optnone push
template <typename T>
auto consume(const T& value) -> T {
  return value;
}
#pragma clang optnone pop
#pragma GCC pop_options

// Multi-threaded map access shouldn't segfault
TEST_P(InputString, MultiThreadedConfigAccess) {
  setLevel(flexi_cfg::logger::Severity::INFO);
  std::atomic_bool done{false};
  constexpr auto thread_count = 6;
  flexi_cfg::logger::info("MultiThreaded Test Start");
  flexi_cfg::Reader cfg({}, "");  // Nominally, we wouldn't do this, but we need a mechanism to
  EXPECT_NO_THROW(cfg = flexi_cfg::Parser::parseFromString(GetParam(), "From String"));
  auto random_read = [&cfg, &done] {
    while (!done) {
      for (const auto& k : cfg.keys()) {
        ASSERT_TRUE(cfg.exists(k));
        switch (cfg.getType(k)) {
          case flexi_cfg::config::types::Type::kString:
            consume(cfg.getValue<std::string>(k));
            break;
          case flexi_cfg::config::types::Type::kNumber:
            consume(cfg.getValue<float>(k));
            break;
          case flexi_cfg::config::types::Type::kBoolean:
            consume(cfg.getValue<bool>(k));
            break;
          case flexi_cfg::config::types::Type::kList: {
            for (const auto& vec = cfg.getValue<std::vector<int>>(k); const auto& v : vec) {
              consume(v);
            }
            break;
          }
          case flexi_cfg::config::types::Type::kExpression:
          case flexi_cfg::config::types::Type::kValueLookup:
          case flexi_cfg::config::types::Type::kVar:
          case flexi_cfg::config::types::Type::kStruct:
          case flexi_cfg::config::types::Type::kStructInProto:
          case flexi_cfg::config::types::Type::kProto:
          case flexi_cfg::config::types::Type::kReference:
          case flexi_cfg::config::types::Type::kUnknown:
          case flexi_cfg::config::types::Type::kValue:
            // These aren't values
            break;
        }
      }
    }
  };

  std::vector<std::thread> threads;
  threads.reserve(thread_count);
  for (auto i = 0; i < thread_count; i++) {
    threads.emplace_back(random_read);
  }
  std::this_thread::sleep_for(std::chrono::seconds(10));  // NOLINT
  done = true;
  for (auto& t : threads) {
    t.join();
  }
  flexi_cfg::logger::info("MultiThreaded Test Done");
}

INSTANTIATE_TEST_SUITE_P(ConfigParse, InputString, testing::Values(std::string(R"(

struct test1 {
    key1 = "value"
    key2 = 1.342    # test comment here
    key3 = 10
    f = "none"
}

reference p as q {
  $A = $(a)
}

a [override] = 2
b [override] = 4

struct test2 {
    my_key = "foo"
    n_key = true

    struct inner {
        list = [1, 2, 3, 4]
        emptyList = []
        listWithComment = [
# I don't matter
        0, 2
        ]
        listWithTrailingComment = [
          0,
          2# I don't matter
        ]
    }
}

a = 1
b = $(a)
c = {{ $(a) }}
d = $(c)
e = {{ $(b) }}
f = $(e)
g = $(a)

proto p {
  e = $A
}

)")));

/// File-based input
class FileInput : public testing::TestWithParam<std::filesystem::path> {};

namespace {
auto baseDir() -> const std::filesystem::path& {
  static const std::filesystem::path base_dir = std::filesystem::path(EXAMPLE_DIR);
  return base_dir;
}

auto filenameGenerator() -> std::vector<std::filesystem::path> {
  std::vector<std::filesystem::path> files;
  for (const auto& entry : std::filesystem::directory_iterator(baseDir())) {
    if (entry.is_regular_file()) {
      if (const auto& file = entry.path().filename().string();
          file.starts_with("config_example") && file.ends_with(".cfg")) {
        files.emplace_back(file);
      }
    }
  }
  std::ranges::sort(files);
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
    flexi_cfg::config::ActionData out{baseDir()};
    return flexi_cfg::config::internal::parseCore<flexi_cfg::config::grammar,
                                                  flexi_cfg::config::action>(in, out);
  };
  bool ret{false};
  EXPECT_NO_THROW(ret = parse());
  EXPECT_TRUE(ret);
}

TEST_P(FileInput, ConfigReaderParse) {
  // This test creates a full path to the config files. This works because they are all top level
  flexi_cfg::logger::setLevel(flexi_cfg::logger::Severity::WARN);
  EXPECT_NO_THROW(flexi_cfg::Parser::parse(baseDir() / GetParam()));
}

TEST_P(FileInput, ConfigReaderParseRootDir) {
  // This test calls parse with a relative path to the config file and specifies the root dir
  flexi_cfg::logger::setLevel(flexi_cfg::logger::Severity::WARN);
  EXPECT_NO_THROW(flexi_cfg::Parser::parse(GetParam(), baseDir()));
}

INSTANTIATE_TEST_SUITE_P(ConfigParse, FileInput, testing::ValuesIn(filenameGenerator()));

TEST(ConfigParse, ConfigRoot) {
  setLevel(flexi_cfg::logger::Severity::DEBUG);
  EXPECT_NO_THROW(flexi_cfg::Parser::parse(
      std::filesystem::path("config_root/test/config_example_base.cfg"), baseDir()));
}

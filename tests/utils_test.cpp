#include "flexi_cfg/utils.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace {
void compareVecEq(const std::vector<std::string>& expected, const std::vector<std::string>& test) {
  ASSERT_EQ(expected.size(), test.size());
  for (size_t i = 0; i < expected.size(); ++i) {
    EXPECT_EQ(expected[i], test[i]);
  }
};
}  // namespace

TEST(UtilsTest, trim) {
  std::string base = "This is a test";

  {
    // Leading whitespace
    std::string test_str = "   " + base;
    EXPECT_EQ(flexi_cfg::utils::trim(test_str), base);
  }
  {
    // Trailing whitespace
    std::string test_str = base + "   \n\n   ";
    EXPECT_EQ(flexi_cfg::utils::trim(test_str), base);
  }
  {
    // Leading and trailing whitespace
    std::string test_str = "   \n  " + base + "   \n\t\t\t   ";
    EXPECT_EQ(flexi_cfg::utils::trim(test_str), base);
  }
  {
    // Leading non-whitespace
    std::string test_str = "{{{" + base;
    EXPECT_EQ(flexi_cfg::utils::trim(test_str, "{"), base);
  }
  {
    // Trailing non-whitespace
    std::string test_str = base + "}}}}}";
    EXPECT_EQ(flexi_cfg::utils::trim(test_str, "}"), base);
  }
  {
    // Enclosing non-whitespace
    std::string test_str = "{{" + base + "}}}}";
    EXPECT_EQ(flexi_cfg::utils::trim(test_str, "{}"), base);
  }
}

TEST(UtilsTest, split) {
  auto combine_str = [](const std::vector<std::string>& in, const std::string& sep) -> std::string {
    std::string combined = in[0];
    for (size_t i = 1; i < in.size(); ++i) {
      combined += sep;
      combined += in[i];
    }
    return combined;
  };

  const std::vector<std::string> parts = {"this", "is", "a", "test"};
  {
    const auto combined = combine_str(parts, ".");
    const auto split = flexi_cfg::utils::split(combined, '.');
    compareVecEq(parts, split);
  }
  {
    const auto combined = combine_str(parts, ";");
    const auto split = flexi_cfg::utils::split(combined, ';');
    compareVecEq(parts, split);
  }
  {
    const auto combined = combine_str(parts, "\t");
    const auto split = flexi_cfg::utils::split(combined, '\t');
    compareVecEq(parts, split);
  }
}

TEST(UtilsTest, join) {
  {
    const std::vector<std::string> input{"this", "is", "a", "test"};
    const std::string expected = "this.is.a.test";

    const auto output = flexi_cfg::utils::join(input, ".");

    EXPECT_EQ(expected, output);
  }
  {
    const std::vector<std::string> input{"just_one"};
    const std::string& expected = input[0];

    const auto output = flexi_cfg::utils::join(input, ".");

    EXPECT_EQ(expected, output);
  }
  {
    const std::string expected{};
    const auto output = flexi_cfg::utils::join({}, ";");

    EXPECT_EQ(expected, output);
  }
}

TEST(UtilsTest, splitAndJoin) {
  {
    const std::vector<std::string> input = {"This", "should", "always", "pass"};

    const auto joined = flexi_cfg::utils::join(input, ".");
    const auto split = flexi_cfg::utils::split(joined, '.');

    compareVecEq(input, split);
  }
  {
    const std::vector<std::string> input = {"one_value"};

    const auto joined = flexi_cfg::utils::join(input, ".");
    const auto split = flexi_cfg::utils::split(joined, '.');

    compareVecEq(input, split);
  }
  {
    const std::vector<std::string> input = {"this.should", "fail"};

    const auto joined = flexi_cfg::utils::join(input, ".");
    const auto split = flexi_cfg::utils::split(joined, '.');
    EXPECT_NE(input.size(), split.size());
  }
}

TEST(UtilsTest, makeName) {
  {
    // Single value first
    const std::string expected = "a_string_here";
    const auto result = flexi_cfg::utils::makeName(expected, "");
    EXPECT_EQ(expected, result);
  }
  {
    // Single value second
    const std::string expected = "a_string_here";
    const auto result = flexi_cfg::utils::makeName("", expected);
    EXPECT_EQ(expected, result);
  }
  {
    // Both values
    const std::string part1 = "first_part";
    const std::string part2 = "second_part";
    const std::string expected = part1 + "." + part2;
    const auto result = flexi_cfg::utils::makeName(part1, part2);
    EXPECT_EQ(expected, result);
  }
  { EXPECT_THROW(flexi_cfg::utils::makeName("", ""), std::runtime_error); }
}

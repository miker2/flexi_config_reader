#include "common/utils.h"

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

// NOLINTNEXTLINE
TEST(utils_test, trim) {
  std::string base = "This is a test";

  {
    // Leading whitespace
    std::string test_str = "   " + base;
    EXPECT_EQ(utils::trim(test_str), base);
  }
  {
    // Trailing whitespace
    std::string test_str = base + "   \n\n   ";
    EXPECT_EQ(utils::trim(test_str), base);
  }
  {
    // Leading and trailing whitespace
    std::string test_str = "   \n  " + base + "   \n\t\t\t   ";
    EXPECT_EQ(utils::trim(test_str), base);
  }
  {
    // Leading non-whitespace
    std::string test_str = "{{{" + base;
    EXPECT_EQ(utils::trim(test_str, "{"), base);
  }
  {
    // Trailing non-whitespace
    std::string test_str = base + "}}}}}";
    EXPECT_EQ(utils::trim(test_str, "}"), base);
  }
  {
    // Enclosing non-whitespace
    std::string test_str = "{{" + base + "}}}}";
    EXPECT_EQ(utils::trim(test_str, "{}"), base);
  }
}

// NOLINTNEXTLINE
TEST(utils_test, split) {
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
    const auto split = utils::split(combined, '.');
    compareVecEq(parts, split);
  }
  {
    const auto combined = combine_str(parts, ";");
    const auto split = utils::split(combined, ';');
    compareVecEq(parts, split);
  }
  {
    const auto combined = combine_str(parts, "\t");
    const auto split = utils::split(combined, '\t');
    compareVecEq(parts, split);
  }
}

// NOLINTNEXTLINE
TEST(utils_test, join) {
  {
    const std::vector<std::string> input{"this", "is", "a", "test"};
    const std::string expected = "this.is.a.test";

    const auto output = utils::join(input, ".");

    EXPECT_EQ(expected, output);
  }
  {
    const std::vector<std::string> input{"just_one"};
    const std::string expected = input[0];

    const auto output = utils::join(input, ".");

    EXPECT_EQ(expected, output);
  }
  {
    const std::string expected{};
    const auto output = utils::join({}, ";");

    EXPECT_EQ(expected, output);
  }
}

// NOLINTNEXTLINE
TEST(utils_test, splitAndJoin) {
  {
    const std::vector<std::string> input = {"This", "should", "always", "pass"};

    const auto joined = utils::join(input, ".");
    const auto split = utils::split(joined, '.');

    compareVecEq(input, split);
  }
  {
    const std::vector<std::string> input = {"one_value"};

    const auto joined = utils::join(input, ".");
    const auto split = utils::split(joined, '.');

    compareVecEq(input, split);
  }
  {
    const std::vector<std::string> input = {"this.should", "fail"};

    const auto joined = utils::join(input, ".");
    const auto split = utils::split(joined, '.');
    EXPECT_NE(input.size(), split.size());
  }
}

// NOLINTNEXTLINE
TEST(utils_test, makeName) {
  {
    // Single value first
    const std::string expected = "a_string_here";
    const auto result = utils::makeName(expected, "");
    EXPECT_EQ(expected, result);
  }
  {
    // Single value second
    const std::string expected = "a_string_here";
    const auto result = utils::makeName("", expected);
    EXPECT_EQ(expected, result);
  }
  {
    // Both values
    const std::string part1 = "first_part";
    const std::string part2 = "second_part";
    const std::string expected = part1 + "." + part2;
    const auto result = utils::makeName(part1, part2);
    EXPECT_EQ(expected, result);
  }
  {
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-goto)
    EXPECT_THROW(utils::makeName("", ""), std::runtime_error);
  }
}

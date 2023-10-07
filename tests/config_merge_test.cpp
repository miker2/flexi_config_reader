#include <gtest/gtest.h>

#include <string>

#include "flexi_cfg/logger.h"
#include "flexi_cfg/parser.h"
#include "flexi_cfg/reader.h"

TEST(ConfigMerge, Merge) {
  flexi_cfg::logger::setLevel(flexi_cfg::logger::Severity::INFO);
  flexi_cfg::Reader base({}, "");
  flexi_cfg::Reader overrides({}, "");
  flexi_cfg::Reader expected({}, "");
  std::string base_config{R"(
key1 = "value"
key2 = "value"
key3 = 10
key4 = false
key5 = [1, 2, 3]
key6 = {{ -pi }}
key7 = $(key1)
struct section1 {
    key8 = "value"
    key9 = false
    key10 = [1, 2, 3]
    struct key11 {
        key12 = false
    }
    key13 = "value"
    key14 = "value"
    key15 = "value"
}
struct section2 {
    key0 = "value"
}
struct section3 {
    key0 = $(key2)
}
struct section4 {
    key0 = false
}
)"};

  std::string override_config{R"(
key0 = "value"
key2 = "override"
key3 = 11
key4 = true
key5 = [4, 5, 6]
key6 = {{ pi }}
struct section1 {
    key8 = "override"
    key9 = true
    key10 = [4, 5, 6]
    struct key11 {
        key12 = true
    }
}
struct section2 {
    key0 = "override"
    key1 = "override"
    key2 = "override"
}
struct section3 {
    key0 = $(key2)
}
)"};

  std::string expected_config{R"(
key0 = "value"
key1 = "value"
key2 = "override"
key3 = 11
key4 = true
key5 = [4, 5, 6]
key6 = {{ pi }}
key7 = "value"
struct section1 {
    key8 = "override"
    key9 = true
    key10 = [4, 5, 6]
    struct key11 {
        key12 = true
    }
    key13 = "value"
    key14 = "value"
    key15 = "value"
}
struct section2 {
    key0 = "override"
    key1 = "override"
    key2 = "override"
}
struct section3 {
    key0 = "override"
}
struct section4 {
    key0 = false
}
)"};

  EXPECT_NO_THROW(base = flexi_cfg::Parser::parse(base_config, "From String"));
  EXPECT_NO_THROW(overrides = flexi_cfg::Parser::parse(override_config, "From String"));
  EXPECT_NO_THROW(base.merge(overrides));
  EXPECT_NO_THROW(expected = flexi_cfg::Parser::parse(expected_config, "From String"));
  EXPECT_EQ(base, expected);
}

TEST(ConfigMerge, ValidOverlay) {
  flexi_cfg::logger::setLevel(flexi_cfg::logger::Severity::INFO);
  flexi_cfg::Reader base({}, "");
  flexi_cfg::Reader overlay({}, "");
  flexi_cfg::Reader expected({}, "");
  std::string base_config{R"(
key1 = "value"
key2 = 10
key3 = false
key4 = [1, 2, 3]
key5 = {{ -pi }}
key6 = $(key1)
struct section1 {
    key8 = "value"
    key9 = false
    key10 = [1, 2, 3]
    struct key11 {
        key12 = false
    }
}
key13 = "untouched"
key14 = "untouched"
)"};

  std::string overlay_config{R"(
key1 = "override"
key2 = 11
key3 = true
key4 = [4, 5, 6]
key5 = {{ pi }}
key6 = "override"
struct section1 {
    key8 = "override"
    key9 = true
    key10 = [4, 5, 6]
    struct key11 {
        key12 = true
    }
}
)"};

  std::string expected_config{R"(
key1 = "override"
key2 = 11
key3 = true
key4 = [4, 5, 6]
key5 = {{ pi }}
key6 = "override"
struct section1 {
    key8 = "override"
    key9 = true
    key10 = [4, 5, 6]
    struct key11 {
        key12 = true
    }
}
key13 = "untouched"
key14 = "untouched"
)"};

  EXPECT_NO_THROW(base = flexi_cfg::Parser::parse(base_config, "From String"));
  EXPECT_NO_THROW(overlay = flexi_cfg::Parser::parse(overlay_config, "From String"));
  EXPECT_NO_THROW(base.applyOverlay(overlay));
  EXPECT_NO_THROW(expected = flexi_cfg::Parser::parse(expected_config, "From String"));
  EXPECT_EQ(base, expected);
}

TEST(ConfigMerge, InvalidOverlayTypeMismatch) {
  flexi_cfg::logger::setLevel(flexi_cfg::logger::Severity::INFO);
  flexi_cfg::Reader base({}, "");
  flexi_cfg::Reader overlay({}, "");
  std::string base_config{R"(
key1 = "value"
)"};

  std::string overlay_config{R"(
key1 = 1234
)"};

  EXPECT_NO_THROW(base = flexi_cfg::Parser::parse(base_config, "From String"));
  EXPECT_NO_THROW(overlay = flexi_cfg::Parser::parse(overlay_config, "From String"));
  EXPECT_THROW(base.applyOverlay(overlay), flexi_cfg::config::MismatchTypeException);
}

TEST(ConfigMerge, InvalidOverlayInvalidKey) {
  flexi_cfg::logger::setLevel(flexi_cfg::logger::Severity::INFO);
  flexi_cfg::Reader base({}, "");
  flexi_cfg::Reader overlay({}, "");
  std::string base_config{R"(
key1 = "value"
)"};

  std::string overlay_config{R"(
nonexistent_key = 1234
)"};

  EXPECT_NO_THROW(base = flexi_cfg::Parser::parse(base_config, "From String"));
  EXPECT_NO_THROW(overlay = flexi_cfg::Parser::parse(overlay_config, "From String"));
  EXPECT_THROW(base.applyOverlay(overlay), flexi_cfg::config::InvalidKeyException);
}

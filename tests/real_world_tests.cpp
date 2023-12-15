#include "lexer.h"
#include "gtest/gtest.h"

TEST(real_world_tests, esbuild_hint_style) {
  auto result = lexer::parse_commonjs("0 && (module.exports = {a, b, c}) && __exportStar(require('fs'));");
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->exports.size(), 3);
  ASSERT_EQ(result->re_exports.size(), 1);
  SUCCEED();
}

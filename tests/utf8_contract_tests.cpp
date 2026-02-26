#include "merve.h"
#include "merve_c.h"

#include "gtest/gtest.h"

#include <cstring>
#include <string_view>

namespace {

bool merve_string_eq_bytes(merve_string s, const char* expected, size_t len) {
  if (s.length != len) return false;
  if (len == 0) return true;
  return std::memcmp(s.data, expected, len) == 0;
}

}  // namespace

TEST(utf8_contract_tests, x_escape_is_utf8_cpp_api) {
  auto result = lexer::parse_commonjs("exports['\\xFF'] = 1;");
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->exports.size(), 1u);

  std::string_view name = lexer::get_string_view(result->exports[0]);
  ASSERT_EQ(name.size(), 2u);
  ASSERT_EQ(static_cast<unsigned char>(name[0]), 0xC3);
  ASSERT_EQ(static_cast<unsigned char>(name[1]), 0xBF);
}

TEST(utf8_contract_tests, x_escape_is_utf8_c_api) {
  const char* source = "exports['\\xFF'] = 1;";
  merve_analysis result = merve_parse_commonjs(source, std::strlen(source));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(merve_is_valid(result));

  constexpr char expected[] = {static_cast<char>(0xC3),
                               static_cast<char>(0xBF)};
  ASSERT_TRUE(merve_string_eq_bytes(merve_get_export_name(result, 0), expected,
                                    sizeof(expected)));
  merve_free(result);
}

TEST(utf8_contract_tests, invalid_utf8_rejected_cpp_api) {
  const char invalid[] = {static_cast<char>(0xFF)};
  auto result =
      lexer::parse_commonjs(std::string_view(invalid, sizeof(invalid)));
  ASSERT_FALSE(result.has_value());

  auto err = lexer::get_last_error();
  ASSERT_TRUE(err.has_value());
  ASSERT_EQ(err.value(), lexer::lexer_error::INVALID_UTF8);
}

TEST(utf8_contract_tests, invalid_utf8_rejected_c_api) {
  const char invalid[] = {static_cast<char>(0xFF)};
  merve_analysis result = merve_parse_commonjs(invalid, sizeof(invalid));
  ASSERT_NE(result, nullptr);
  ASSERT_FALSE(merve_is_valid(result));
  ASSERT_EQ(merve_get_last_error(), MERVE_ERROR_INVALID_UTF8);
  merve_free(result);
}

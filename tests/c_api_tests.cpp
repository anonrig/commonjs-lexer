#include "merve_c.h"

#include "gtest/gtest.h"
#include <cstring>

// Helper: compare merve_string to a C string literal.
static bool merve_string_eq(merve_string s, const char* expected) {
  size_t expected_len = std::strlen(expected);
  if (s.length != expected_len) return false;
  if (expected_len == 0) return true;
  return std::memcmp(s.data, expected, expected_len) == 0;
}

TEST(c_api_tests, version_string) {
  const char* version = merve_get_version();
  ASSERT_NE(version, nullptr);
  // Just verify it returns a non-empty string.
  ASSERT_GT(std::strlen(version), 0u);
}

TEST(c_api_tests, version_components) {
  merve_version_components vc = merve_get_version_components();
  ASSERT_GE(vc.major, 1);
  ASSERT_GE(vc.minor, 0);
  ASSERT_GE(vc.revision, 0);
}

TEST(c_api_tests, basic_exports) {
  const char* source = "exports.foo = 1; exports.bar = 2;";
  merve_analysis result = merve_parse_commonjs(source, std::strlen(source));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(merve_is_valid(result));
  ASSERT_EQ(merve_get_exports_count(result), 2u);
  ASSERT_EQ(merve_get_reexports_count(result), 0u);
  ASSERT_TRUE(merve_string_eq(merve_get_export_name(result, 0), "foo"));
  ASSERT_TRUE(merve_string_eq(merve_get_export_name(result, 1), "bar"));
  merve_free(result);
}

TEST(c_api_tests, module_exports_dot) {
  const char* source = "module.exports.asdf = 'asdf';";
  merve_analysis result = merve_parse_commonjs(source, std::strlen(source));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(merve_is_valid(result));
  ASSERT_EQ(merve_get_exports_count(result), 1u);
  ASSERT_TRUE(merve_string_eq(merve_get_export_name(result, 0), "asdf"));
  merve_free(result);
}

TEST(c_api_tests, module_exports_literal) {
  const char* source = "module.exports = { a, b, c };";
  merve_analysis result = merve_parse_commonjs(source, std::strlen(source));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(merve_is_valid(result));
  ASSERT_EQ(merve_get_exports_count(result), 3u);
  ASSERT_TRUE(merve_string_eq(merve_get_export_name(result, 0), "a"));
  ASSERT_TRUE(merve_string_eq(merve_get_export_name(result, 1), "b"));
  ASSERT_TRUE(merve_string_eq(merve_get_export_name(result, 2), "c"));
  merve_free(result);
}

TEST(c_api_tests, reexport) {
  const char* source = "module.exports = require('./dep');";
  merve_analysis result = merve_parse_commonjs(source, std::strlen(source));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(merve_is_valid(result));
  ASSERT_EQ(merve_get_exports_count(result), 0u);
  ASSERT_EQ(merve_get_reexports_count(result), 1u);
  ASSERT_TRUE(merve_string_eq(merve_get_reexport_name(result, 0), "./dep"));
  merve_free(result);
}

TEST(c_api_tests, multiple_reexports) {
  const char* source =
      "__exportStar(require('a')); __exportStar(require('b'));";
  merve_analysis result = merve_parse_commonjs(source, std::strlen(source));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(merve_is_valid(result));
  ASSERT_EQ(merve_get_reexports_count(result), 2u);
  ASSERT_TRUE(merve_string_eq(merve_get_reexport_name(result, 0), "a"));
  ASSERT_TRUE(merve_string_eq(merve_get_reexport_name(result, 1), "b"));
  merve_free(result);
}

TEST(c_api_tests, esbuild_hint_style) {
  const char* source =
      "0 && (module.exports = {a, b, c}) && __exportStar(require('fs'));";
  merve_analysis result = merve_parse_commonjs(source, std::strlen(source));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(merve_is_valid(result));
  ASSERT_EQ(merve_get_exports_count(result), 3u);
  ASSERT_EQ(merve_get_reexports_count(result), 1u);
  ASSERT_TRUE(merve_string_eq(merve_get_export_name(result, 0), "a"));
  ASSERT_TRUE(merve_string_eq(merve_get_export_name(result, 1), "b"));
  ASSERT_TRUE(merve_string_eq(merve_get_export_name(result, 2), "c"));
  ASSERT_TRUE(merve_string_eq(merve_get_reexport_name(result, 0), "fs"));
  merve_free(result);
}

TEST(c_api_tests, esm_import_error) {
  const char* source = "import 'x';";
  merve_analysis result = merve_parse_commonjs(source, std::strlen(source));
  ASSERT_NE(result, nullptr);
  ASSERT_FALSE(merve_is_valid(result));
  ASSERT_EQ(merve_get_last_error(), MERVE_ERROR_UNEXPECTED_ESM_IMPORT);
  merve_free(result);
}

TEST(c_api_tests, esm_export_error) {
  const char* source = "export { x };";
  merve_analysis result = merve_parse_commonjs(source, std::strlen(source));
  ASSERT_NE(result, nullptr);
  ASSERT_FALSE(merve_is_valid(result));
  ASSERT_EQ(merve_get_last_error(), MERVE_ERROR_UNEXPECTED_ESM_EXPORT);
  merve_free(result);
}

TEST(c_api_tests, import_meta_error) {
  const char* source = "import.meta.url";
  merve_analysis result = merve_parse_commonjs(source, std::strlen(source));
  ASSERT_NE(result, nullptr);
  ASSERT_FALSE(merve_is_valid(result));
  ASSERT_EQ(merve_get_last_error(), MERVE_ERROR_UNEXPECTED_ESM_IMPORT_META);
  merve_free(result);
}

TEST(c_api_tests, no_error_after_success) {
  const char* source = "exports.x = 1;";
  merve_analysis result = merve_parse_commonjs(source, std::strlen(source));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(merve_is_valid(result));
  ASSERT_EQ(merve_get_last_error(), -1);
  merve_free(result);
}

TEST(c_api_tests, empty_input) {
  merve_analysis result = merve_parse_commonjs("", 0);
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(merve_is_valid(result));
  ASSERT_EQ(merve_get_exports_count(result), 0u);
  ASSERT_EQ(merve_get_reexports_count(result), 0u);
  merve_free(result);
}

TEST(c_api_tests, null_input) {
  merve_analysis result = merve_parse_commonjs(NULL, 0);
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(merve_is_valid(result));
  ASSERT_EQ(merve_get_exports_count(result), 0u);
  merve_free(result);
}

TEST(c_api_tests, null_handle_safety) {
  ASSERT_FALSE(merve_is_valid(NULL));
  ASSERT_EQ(merve_get_exports_count(NULL), 0u);
  ASSERT_EQ(merve_get_reexports_count(NULL), 0u);

  merve_string s = merve_get_export_name(NULL, 0);
  ASSERT_EQ(s.data, nullptr);
  ASSERT_EQ(s.length, 0u);
  ASSERT_EQ(merve_get_export_line(NULL, 0), 0u);

  s = merve_get_reexport_name(NULL, 0);
  ASSERT_EQ(s.data, nullptr);
  ASSERT_EQ(s.length, 0u);
  ASSERT_EQ(merve_get_reexport_line(NULL, 0), 0u);

  merve_free(NULL);  // must not crash
}

TEST(c_api_tests, out_of_bounds_access) {
  const char* source = "exports.x = 1;";
  merve_analysis result = merve_parse_commonjs(source, std::strlen(source));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(merve_is_valid(result));
  ASSERT_EQ(merve_get_exports_count(result), 1u);

  // Valid access
  ASSERT_TRUE(merve_string_eq(merve_get_export_name(result, 0), "x"));
  ASSERT_NE(merve_get_export_line(result, 0), 0u);

  // Out of bounds exports
  merve_string s = merve_get_export_name(result, 1);
  ASSERT_EQ(s.data, nullptr);
  ASSERT_EQ(s.length, 0u);
  ASSERT_EQ(merve_get_export_line(result, 1), 0u);

  s = merve_get_export_name(result, 999);
  ASSERT_EQ(s.data, nullptr);
  ASSERT_EQ(s.length, 0u);

  // Out of bounds re-exports (none exist)
  s = merve_get_reexport_name(result, 0);
  ASSERT_EQ(s.data, nullptr);
  ASSERT_EQ(s.length, 0u);
  ASSERT_EQ(merve_get_reexport_line(result, 0), 0u);

  merve_free(result);
}

TEST(c_api_tests, invalid_result_accessors) {
  // Parse ESM to get an invalid result.
  const char* source = "import 'x';";
  merve_analysis result = merve_parse_commonjs(source, std::strlen(source));
  ASSERT_NE(result, nullptr);
  ASSERT_FALSE(merve_is_valid(result));

  ASSERT_EQ(merve_get_exports_count(result), 0u);
  ASSERT_EQ(merve_get_reexports_count(result), 0u);

  merve_string s = merve_get_export_name(result, 0);
  ASSERT_EQ(s.data, nullptr);
  ASSERT_EQ(s.length, 0u);
  ASSERT_EQ(merve_get_export_line(result, 0), 0u);

  s = merve_get_reexport_name(result, 0);
  ASSERT_EQ(s.data, nullptr);
  ASSERT_EQ(s.length, 0u);
  ASSERT_EQ(merve_get_reexport_line(result, 0), 0u);

  merve_free(result);
}

TEST(c_api_tests, line_numbers) {
  const char* source =
      "// line 1\n"
      "exports.a = 1;\n"
      "\n"
      "exports.b = 2;\n";
  merve_analysis result = merve_parse_commonjs(source, std::strlen(source));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(merve_is_valid(result));
  ASSERT_EQ(merve_get_exports_count(result), 2u);
  ASSERT_TRUE(merve_string_eq(merve_get_export_name(result, 0), "a"));
  ASSERT_EQ(merve_get_export_line(result, 0), 2u);
  ASSERT_TRUE(merve_string_eq(merve_get_export_name(result, 1), "b"));
  ASSERT_EQ(merve_get_export_line(result, 1), 4u);
  merve_free(result);
}

TEST(c_api_tests, reexport_line_numbers) {
  const char* source =
      "// line 1\n"
      "module.exports = require('dep1');\n";
  merve_analysis result = merve_parse_commonjs(source, std::strlen(source));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(merve_is_valid(result));
  ASSERT_EQ(merve_get_reexports_count(result), 1u);
  ASSERT_TRUE(merve_string_eq(merve_get_reexport_name(result, 0), "dep1"));
  ASSERT_EQ(merve_get_reexport_line(result, 0), 2u);
  merve_free(result);
}

TEST(c_api_tests, bracket_notation_exports) {
  const char* source = "exports['not identifier'] = 1;";
  merve_analysis result = merve_parse_commonjs(source, std::strlen(source));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(merve_is_valid(result));
  ASSERT_EQ(merve_get_exports_count(result), 1u);
  ASSERT_TRUE(
      merve_string_eq(merve_get_export_name(result, 0), "not identifier"));
  merve_free(result);
}

TEST(c_api_tests, define_property_exports) {
  const char* source =
      "Object.defineProperty(module.exports, 'thing', { value: true });\n"
      "Object.defineProperty(exports, 'other', { enumerable: true, value: "
      "true });";
  merve_analysis result = merve_parse_commonjs(source, std::strlen(source));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(merve_is_valid(result));
  ASSERT_EQ(merve_get_exports_count(result), 2u);
  ASSERT_TRUE(merve_string_eq(merve_get_export_name(result, 0), "thing"));
  ASSERT_TRUE(merve_string_eq(merve_get_export_name(result, 1), "other"));
  merve_free(result);
}

TEST(c_api_tests, whitespace_only) {
  const char* source = "   \n\t\r\n   ";
  merve_analysis result = merve_parse_commonjs(source, std::strlen(source));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(merve_is_valid(result));
  ASSERT_EQ(merve_get_exports_count(result), 0u);
  ASSERT_EQ(merve_get_reexports_count(result), 0u);
  merve_free(result);
}

TEST(c_api_tests, typescript_reexports) {
  const char* source =
      "\"use strict\";\n"
      "__export(require(\"external1\"));\n"
      "__exportStar(require(\"external2\"));\n";
  merve_analysis result = merve_parse_commonjs(source, std::strlen(source));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(merve_is_valid(result));
  ASSERT_EQ(merve_get_reexports_count(result), 2u);
  ASSERT_TRUE(
      merve_string_eq(merve_get_reexport_name(result, 0), "external1"));
  ASSERT_TRUE(
      merve_string_eq(merve_get_reexport_name(result, 1), "external2"));
  merve_free(result);
}

TEST(c_api_tests, shebang) {
  const char* source = "#! (  {\n      exports.asdf = 'asdf';\n    ";
  merve_analysis result = merve_parse_commonjs(source, std::strlen(source));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(merve_is_valid(result));
  ASSERT_EQ(merve_get_exports_count(result), 1u);
  ASSERT_TRUE(merve_string_eq(merve_get_export_name(result, 0), "asdf"));
  merve_free(result);
}

TEST(c_api_tests, conditional_exports) {
  const char* source =
      "if (condition) {\n"
      "  exports.a = 1;\n"
      "} else {\n"
      "  exports.b = 2;\n"
      "}\n";
  merve_analysis result = merve_parse_commonjs(source, std::strlen(source));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(merve_is_valid(result));
  ASSERT_EQ(merve_get_exports_count(result), 2u);
  ASSERT_TRUE(merve_string_eq(merve_get_export_name(result, 0), "a"));
  ASSERT_TRUE(merve_string_eq(merve_get_export_name(result, 1), "b"));
  merve_free(result);
}

TEST(c_api_tests, multiple_independent_parses) {
  // Verify handles are independent.
  const char* source1 = "exports.x = 1;";
  const char* source2 = "exports.y = 1; exports.z = 2;";

  merve_analysis r1 = merve_parse_commonjs(source1, std::strlen(source1));
  merve_analysis r2 = merve_parse_commonjs(source2, std::strlen(source2));

  ASSERT_NE(r1, nullptr);
  ASSERT_NE(r2, nullptr);
  ASSERT_NE(r1, r2);

  ASSERT_EQ(merve_get_exports_count(r1), 1u);
  ASSERT_EQ(merve_get_exports_count(r2), 2u);

  ASSERT_TRUE(merve_string_eq(merve_get_export_name(r1, 0), "x"));
  ASSERT_TRUE(merve_string_eq(merve_get_export_name(r2, 0), "y"));
  ASSERT_TRUE(merve_string_eq(merve_get_export_name(r2, 1), "z"));

  merve_free(r1);
  merve_free(r2);
}

TEST(c_api_tests, non_identifiers) {
  const char* source =
      "module.exports = { 'ab cd': foo };\n"
      "exports['@notidentifier'] = 'asdf';\n";
  merve_analysis result = merve_parse_commonjs(source, std::strlen(source));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(merve_is_valid(result));
  ASSERT_EQ(merve_get_exports_count(result), 2u);
  ASSERT_TRUE(merve_string_eq(merve_get_export_name(result, 0), "ab cd"));
  ASSERT_TRUE(
      merve_string_eq(merve_get_export_name(result, 1), "@notidentifier"));
  merve_free(result);
}

TEST(c_api_tests, spread_reexports) {
  const char* source =
      "module.exports = {\n"
      "  ...require('dep1'),\n"
      "  name,\n"
      "  ...require('dep2'),\n"
      "};\n";
  merve_analysis result = merve_parse_commonjs(source, std::strlen(source));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(merve_is_valid(result));
  ASSERT_EQ(merve_get_exports_count(result), 1u);
  ASSERT_TRUE(merve_string_eq(merve_get_export_name(result, 0), "name"));
  ASSERT_EQ(merve_get_reexports_count(result), 2u);
  ASSERT_TRUE(merve_string_eq(merve_get_reexport_name(result, 0), "dep1"));
  ASSERT_TRUE(merve_string_eq(merve_get_reexport_name(result, 1), "dep2"));
  merve_free(result);
}

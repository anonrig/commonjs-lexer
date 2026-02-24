/**
 * @file c_api_compile_test.c
 * @brief Verifies that merve_c.h compiles cleanly as pure C.
 *
 * This is a compile-only test. It does not call any functions;
 * it merely checks that the types and declarations parse under a C compiler.
 */
#include "merve_c.h"

/* Exercise every typedef so the compiler sees them. */
static void check_types(void) {
  merve_string s;
  s.data = "hello";
  s.length = 5;
  (void)s;

  merve_version_components vc;
  vc.major = 1;
  vc.minor = 0;
  vc.revision = 1;
  (void)vc;

  merve_analysis a = (merve_analysis)0;
  (void)a;

  /* Verify the error constants are valid integer constant expressions. */
  int errors[] = {
      MERVE_ERROR_TODO,
      MERVE_ERROR_UNEXPECTED_PAREN,
      MERVE_ERROR_UNEXPECTED_BRACE,
      MERVE_ERROR_UNTERMINATED_PAREN,
      MERVE_ERROR_UNTERMINATED_BRACE,
      MERVE_ERROR_UNTERMINATED_TEMPLATE_STRING,
      MERVE_ERROR_UNTERMINATED_STRING_LITERAL,
      MERVE_ERROR_UNTERMINATED_REGEX_CHARACTER_CLASS,
      MERVE_ERROR_UNTERMINATED_REGEX,
      MERVE_ERROR_UNEXPECTED_ESM_IMPORT_META,
      MERVE_ERROR_UNEXPECTED_ESM_IMPORT,
      MERVE_ERROR_UNEXPECTED_ESM_EXPORT,
      MERVE_ERROR_TEMPLATE_NEST_OVERFLOW,
  };
  (void)errors;
}

int main(void) {
  check_types();
  return 0;
}

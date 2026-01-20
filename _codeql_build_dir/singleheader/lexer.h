/* auto-generated on 2026-01-15 02:22:18 +0000. Do not edit! */
/* begin file include/lexer.h */
#ifndef LEXER_H
#define LEXER_H

/* begin file include/lexer/parser.h */
#ifndef LEXER_PARSER_H
#define LEXER_PARSER_H

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace lexer {

  enum lexer_error {
    // remove this error when implementation is complete
    TODO,
    // syntax errors
    UNEXPECTED_PAREN,
    UNEXPECTED_BRACE,
    UNTERMINATED_PAREN,
    UNTERMINATED_BRACE,
    UNTERMINATED_TEMPLATE_STRING,
    UNTERMINATED_STRING_LITERAL,
    UNTERMINATED_REGEX_CHARACTER_CLASS,
    UNTERMINATED_REGEX,

    // ESM syntax errors
    UNEXPECTED_ESM_IMPORT_META,
    UNEXPECTED_ESM_IMPORT,
    UNEXPECTED_ESM_EXPORT,

    // overflows
    // todo - we need to extend overflow checks to all data types
    TEMPLATE_NEST_OVERFLOW,
  };

  struct lexer_analysis {
    std::vector<std::string> exports;
    std::vector<std::string> re_exports;
  };

  std::optional<lexer_analysis> parse_commonjs(std::string_view file_contents);
  std::optional<lexer_error> get_last_error();
}

#endif  // LEXER_PARSER_H
/* end file include/lexer/parser.h */

#endif  // LEXER_H
/* end file include/lexer.h */

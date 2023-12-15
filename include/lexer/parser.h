#ifndef LEXER_PARSER_H
#define LEXER_PARSER_H

#include <optional>
#include <string_view>
#include <vector>

namespace lexer {

  struct lexer_result {
    std::vector<std::string_view> exports;
    std::vector<std::string_view> re_exports;
  };

  std::optional<lexer_result> parse_commonjs(std::string_view file_contents);

}

#endif  // LEXER_PARSER_H

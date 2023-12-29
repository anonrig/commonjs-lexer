#include "lexer/parser.h"
#include <optional>

namespace lexer {
  std::optional<lexer_error> last_error;

  // TODO: determine if we want to parse on UTF16 or UTF8 source
  // former implementation parses on UTF16 source, which may be preferable
  // for performance, but this needs to be investigated further.
  std::optional<lexer_analysis> parse_commonjs(const std::string_view file_contents) {
    last_error.reset();
    // TODO: implementation
    last_error.emplace(lexer_error::TODO);
    return std::nullopt;
  }

  std::optional<lexer_error> get_last_error() {
    return last_error;
  }
}

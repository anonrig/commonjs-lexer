#include "lexer/parser.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <limits>

namespace lexer {

// Stack depth limits
constexpr size_t STACK_DEPTH = 2048;
constexpr size_t MAX_STAR_EXPORTS = 256;

// RequireType enum for parsing require statements
enum class RequireType {
  Import,
  ExportAssign,
  ExportStar
};

// StarExportBinding structure for tracking star export bindings
struct StarExportBinding {
  const char* specifier_start;
  const char* specifier_end;
  const char* id_start;
  const char* id_end;
};

// Lexer state class
class CJSLexer {
private:
  const char* source;
  const char* pos;
  const char* end;
  const char* lastTokenPos;

  uint16_t templateStackDepth;
  uint16_t openTokenDepth;
  uint16_t templateDepth;
  uint16_t braceDepth;

  bool lastSlashWasDivision;
  bool nextBraceIsClass;

  std::array<uint16_t, STACK_DEPTH> templateStack_;
  std::array<const char*, STACK_DEPTH> openTokenPosStack_;
  std::array<bool, STACK_DEPTH> openClassPosStack;
  std::array<StarExportBinding, MAX_STAR_EXPORTS> starExportStack_;
  StarExportBinding* starExportStack;
  const StarExportBinding* STAR_EXPORT_STACK_END;

  std::vector<std::string>* exports;
  std::vector<std::string>* re_exports;

  std::optional<lexer_error> parse_error;

  // Character classification helpers
  static constexpr bool isBr(char c) {
    return c == '\r' || c == '\n';
  }

  static constexpr bool isBrOrWs(char c) {
    return (c > 8 && c < 14) || c == 32 || c == '\t';
  }

  static constexpr bool isBrOrWsOrPunctuatorNotDot(char c) {
    return isBrOrWs(c) || (isPunctuator(c) && c != '.');
  }

  static constexpr bool isPunctuator(char ch) {
    return ch == '!' || ch == '%' || ch == '&' ||
           (ch > 39 && ch < 48) || (ch > 57 && ch < 64) ||
           ch == '[' || ch == ']' || ch == '^' ||
           (ch > 122 && ch < 127);
  }

  static constexpr bool isExpressionPunctuator(char ch) {
    return ch == '!' || ch == '%' || ch == '&' ||
           (ch > 39 && ch < 47 && ch != 41) || (ch > 57 && ch < 64) ||
           ch == '[' || ch == '^' || (ch > 122 && ch < 127 && ch != '}');
  }

  // String comparison helpers using string_view for cleaner, more maintainable code
  bool matchesAt(const char* p, const char* end_pos, std::string_view expected) const {
    size_t available = end_pos - p;
    if (available < expected.size()) return false;
    return std::string_view(p, expected.size()) == expected;
  }

  // Character type detection - simplified for ASCII/UTF-8
  static bool isIdentifierStart(uint8_t ch) {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_' || ch == '$' || ch >= 0x80;
  }

  static bool isIdentifierChar(uint8_t ch) {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
           (ch >= '0' && ch <= '9') || ch == '_' || ch == '$' || ch >= 0x80;
  }

  bool keywordStart(const char* p) const {
    return p == source || isBrOrWsOrPunctuatorNotDot(*(p - 1));
  }

  bool readPrecedingKeyword2(const char* p, std::string_view keyword) const {
    if (p - static_cast<ptrdiff_t>(keyword.size()) + 1 < source) return false;
    const char* start = p - keyword.size() + 1;
    return matchesAt(start, end, keyword) && (start == source || isBrOrWsOrPunctuatorNotDot(*(start - 1)));
  }

  bool readPrecedingKeyword3(const char* p, std::string_view keyword) const {
    if (p - static_cast<ptrdiff_t>(keyword.size()) + 1 < source) return false;
    const char* start = p - keyword.size() + 1;
    return matchesAt(start, end, keyword) && (start == source || isBrOrWsOrPunctuatorNotDot(*(start - 1)));
  }

  bool readPrecedingKeyword4(const char* p, std::string_view keyword) const {
    if (p - static_cast<ptrdiff_t>(keyword.size()) + 1 < source) return false;
    const char* start = p - keyword.size() + 1;
    return matchesAt(start, end, keyword) && (start == source || isBrOrWsOrPunctuatorNotDot(*(start - 1)));
  }

  bool readPrecedingKeyword5(const char* p, std::string_view keyword) const {
    if (p - static_cast<ptrdiff_t>(keyword.size()) + 1 < source) return false;
    const char* start = p - keyword.size() + 1;
    return matchesAt(start, end, keyword) && (start == source || isBrOrWsOrPunctuatorNotDot(*(start - 1)));
  }

  bool readPrecedingKeyword6(const char* p, std::string_view keyword) const {
    if (p - static_cast<ptrdiff_t>(keyword.size()) + 1 < source) return false;
    const char* start = p - keyword.size() + 1;
    return matchesAt(start, end, keyword) && (start == source || isBrOrWsOrPunctuatorNotDot(*(start - 1)));
  }

  bool readPrecedingKeyword7(const char* p, std::string_view keyword) const {
    if (p - static_cast<ptrdiff_t>(keyword.size()) + 1 < source) return false;
    const char* start = p - keyword.size() + 1;
    return matchesAt(start, end, keyword) && (start == source || isBrOrWsOrPunctuatorNotDot(*(start - 1)));
  }

  // Keyword detection
  bool isExpressionKeyword(const char* p) const {
    switch (*p) {
      case 'd':
        switch (*(p - 1)) {
          case 'i':
            return readPrecedingKeyword2(p - 2, "vo");
          case 'l':
            return readPrecedingKeyword3(p - 2, "yie");
          default:
            return false;
        }
      case 'e':
        switch (*(p - 1)) {
          case 's':
            switch (*(p - 2)) {
              case 'l':
                return p - 3 >= source && *(p - 3) == 'e' && keywordStart(p - 3);
              case 'a':
                return p - 3 >= source && *(p - 3) == 'c' && keywordStart(p - 3);
              default:
                return false;
            }
          case 't':
            return readPrecedingKeyword4(p - 2, "dele");
          default:
            return false;
        }
      case 'f':
        if (*(p - 1) != 'o' || *(p - 2) != 'e')
          return false;
        switch (*(p - 3)) {
          case 'c':
            return readPrecedingKeyword6(p - 4, "instan");
          case 'p':
            return readPrecedingKeyword2(p - 4, "ty");
          default:
            return false;
        }
      case 'n':
        return (p - 1 >= source && *(p - 1) == 'i' && keywordStart(p - 1)) ||
               readPrecedingKeyword5(p - 1, "retur");
      case 'o':
        return p - 1 >= source && *(p - 1) == 'd' && keywordStart(p - 1);
      case 'r':
        return readPrecedingKeyword7(p - 1, "debugge");
      case 't':
        return readPrecedingKeyword4(p - 1, "awai");
      case 'w':
        switch (*(p - 1)) {
          case 'e':
            return p - 2 >= source && *(p - 2) == 'n' && keywordStart(p - 2);
          case 'o':
            return readPrecedingKeyword3(p - 2, "thr");
          default:
            return false;
        }
    }
    return false;
  }

  bool isParenKeyword(const char* curPos) const {
    return readPrecedingKeyword5(curPos, "while") ||
           readPrecedingKeyword3(curPos, "for") ||
           readPrecedingKeyword2(curPos, "if");
  }

  bool isExpressionTerminator(const char* curPos) const {
    switch (*curPos) {
      case '>':
        return *(curPos - 1) == '=';
      case ';':
      case ')':
        return true;
      case 'h':
        return readPrecedingKeyword4(curPos - 1, "catc");
      case 'y':
        return readPrecedingKeyword6(curPos - 1, "finall");
      case 'e':
        return readPrecedingKeyword3(curPos - 1, "els");
    }
    return false;
  }

  // Parsing utilities
  void syntaxError(lexer_error code) {
    if (!parse_error) {
      parse_error = code;
    }
    pos = end + 1;
  }

  char commentWhitespace() {
    char ch;
    do {
      if (pos >= end) return '\0';
      ch = *pos;
      if (ch == '/') {
        char next_ch = pos + 1 < end ? *(pos + 1) : '\0';
        if (next_ch == '/')
          lineComment();
        else if (next_ch == '*')
          blockComment();
        else
          return ch;
      } else if (!isBrOrWs(ch)) {
        return ch;
      }
    } while (pos++ < end);
    return ch;
  }

  void lineComment() {
    while (pos++ < end) {
      char ch = *pos;
      if (ch == '\n' || ch == '\r')
        return;
    }
  }

  void blockComment() {
    pos++;
    while (pos++ < end) {
      char ch = *pos;
      if (ch == '*' && *(pos + 1) == '/') {
        pos++;
        return;
      }
    }
  }

  void stringLiteral(char quote) {
    while (pos++ < end) {
      char ch = *pos;
      if (ch == quote)
        return;
      if (ch == '\\') {
        if (pos + 1 >= end) break;
        ch = *++pos;
        if (ch == '\r' && *(pos + 1) == '\n')
          pos++;
      } else if (isBr(ch))
        break;
    }
    syntaxError(lexer_error::UNTERMINATED_STRING_LITERAL);
  }

  void regularExpression() {
    while (pos++ < end) {
      char ch = *pos;
      if (ch == '/')
        return;
      if (ch == '[') {
        regexCharacterClass();
      } else if (ch == '\\') {
        if (pos + 1 < end)
          pos++;
      } else if (ch == '\n' || ch == '\r')
        break;
    }
    syntaxError(lexer_error::UNTERMINATED_REGEX);
  }

  void regexCharacterClass() {
    while (pos++ < end) {
      char ch = *pos;
      if (ch == ']')
        return;
      if (ch == '\\') {
        if (pos + 1 < end)
          pos++;
      } else if (ch == '\n' || ch == '\r')
        break;
    }
    syntaxError(lexer_error::UNTERMINATED_REGEX_CHARACTER_CLASS);
  }

  void templateString() {
    while (pos++ < end) {
      char ch = *pos;
      if (ch == '$' && *(pos + 1) == '{') {
        pos++;
        if (templateStackDepth >= STACK_DEPTH) {
          syntaxError(lexer_error::TEMPLATE_NEST_OVERFLOW);
          return;
        }
        templateStack_[templateStackDepth++] = templateDepth;
        templateDepth = ++openTokenDepth;
        return;
      }
      if (ch == '`')
        return;
      if (ch == '\\' && pos + 1 < end)
        pos++;
    }
    syntaxError(lexer_error::UNTERMINATED_TEMPLATE_STRING);
  }

  bool identifier(char startCh) {
    if (!isIdentifierStart(static_cast<uint8_t>(startCh)))
      return false;
    pos++;
    while (pos < end) {
      char ch = *pos;
      if (isIdentifierChar(static_cast<uint8_t>(ch))) {
        pos++;
      } else {
        break;
      }
    }
    return true;
  }

  void addExport(const char* start, const char* end_pos) {
    // Skip surrounding quotes if present
    if (start < end_pos && (*start == '\'' || *start == '"')) {
      start++;
      end_pos--;
    }
    // Create string_view to check for duplicates without allocation
    std::string_view export_name(start, end_pos - start);
    
    // Skip exports that are incomplete Unicode escape sequences
    // A single \u{XXXX} is 8 chars: \u{D83C}
    // Complete emoji like \u{D83C}\u{DF10} is 16 chars
    // Filter out single surrogate halves which are invalid on their own
    if (export_name.size() == 8 &&
        export_name[0] == '\\' && export_name[1] == 'u' && export_name[2] == '{' &&
        export_name[7] == '}') {
      // Check if it's in surrogate pair range (D800-DFFF)
      if (export_name[3] == 'D' && 
          ((export_name[4] >= '8' && export_name[4] <= '9') ||
           (export_name[4] >= 'A' && export_name[4] <= 'F'))) {
        return; // Skip incomplete surrogate pairs
      }
    }
    
    // Check if this export already exists (avoid duplicates)
    for (const auto& existing : *exports) {
      if (existing == export_name) {
        return; // Already exists, skip
      }
    }
    exports->emplace_back(start, end_pos - start);
  }

  void addReexport(const char* start, const char* end_pos) {
    // Skip surrounding quotes if present
    if (start < end_pos && (*start == '\'' || *start == '"')) {
      start++;
      end_pos--;
    }
    re_exports->emplace_back(start, end_pos - start);
  }

  void clearReexports() {
    re_exports->clear();
  }

  bool readExportsOrModuleDotExports(char ch) {
    const char* revertPos = pos;
    if (ch == 'm' && matchesAt(pos + 1, end, "odule")) {
      pos += 6;
      ch = commentWhitespace();
      if (ch != '.') {
        pos = revertPos;
        return false;
      }
      pos++;
      ch = commentWhitespace();
    }
    if (ch == 'e' && matchesAt(pos + 1, end, "xports")) {
      pos += 7;
      return true;
    }
    pos = revertPos;
    return false;
  }

  bool tryParseRequire(RequireType requireType) {
    const char* revertPos = pos;
    if (!matchesAt(pos + 1, end, "equire")) {
      return false;
    }
    pos += 7;
    char ch = commentWhitespace();
    if (ch == '(') {
      pos++;
      ch = commentWhitespace();
      const char* reexportStart = pos;
      if (ch == '\'' || ch == '"') {
        stringLiteral(ch);
        const char* reexportEnd = ++pos;
        ch = commentWhitespace();
        if (ch == ')') {
          switch (requireType) {
            case RequireType::ExportStar:
            case RequireType::ExportAssign:
              addReexport(reexportStart, reexportEnd);
              return true;
            default:
              if (starExportStack < STAR_EXPORT_STACK_END) {
                starExportStack->specifier_start = reexportStart;
                starExportStack->specifier_end = reexportEnd;
              }
              return true;
          }
        }
      }
    }
    pos = revertPos;
    return false;
  }

  // Helper to parse property value in object literal (identifier or require())
  bool tryParsePropertyValue(char& ch) {
    if (ch == 'r' && tryParseRequire(RequireType::ExportAssign)) {
      ch = *pos;
      return true;
    }
    if (identifier(ch)) {
      ch = *pos;
      return true;
    }
    return false;
  }

  void tryParseLiteralExports() {
    const char* revertPos = pos - 1;
    while (pos++ < end) {
      char ch = commentWhitespace();
      const char* startPos = pos;
      if (identifier(ch)) {
        const char* endPos = pos;
        ch = commentWhitespace();
        
        // Check if this is a getter syntax: get identifier()
        if (ch != ':' && endPos - startPos == 3 && matchesAt(startPos, end, "get")) {
          // Skip getter: get identifier() { ... }
          if (identifier(ch)) {
            ch = commentWhitespace();
            if (ch == '(') {
              // This is a getter, stop parsing here (early termination)
              pos = revertPos;
              return;
            }
          }
          // Not a getter, revert and fail
          pos = revertPos;
          return;
        }
        
        if (ch == ':') {
          pos++;
          ch = commentWhitespace();
          if (!tryParsePropertyValue(ch)) {
            pos = revertPos;
            return;
          }
        }
        addExport(startPos, endPos);
      } else if (ch == '\'' || ch == '"') {
        const char* start = pos;
        stringLiteral(ch);
        const char* end_pos = ++pos;
        ch = commentWhitespace();
        if (ch == ':') {
          pos++;
          ch = commentWhitespace();
          if (!tryParsePropertyValue(ch)) {
            pos = revertPos;
            return;
          }
          addExport(start, end_pos);
        }
      } else if (ch == '.' && matchesAt(pos + 1, end, "..")) {
        pos += 3;
        if (pos < end && *pos == 'r' && tryParseRequire(RequireType::ExportAssign)) {
          pos++;
        } else if (pos < end && !identifier(*pos)) {
          pos = revertPos;
          return;
        }
        ch = commentWhitespace();
      } else {
        pos = revertPos;
        return;
      }

      if (ch == '}')
        return;

      if (ch != ',') {
        pos = revertPos;
        return;
      }
    }
  }

  void tryParseExportsDotAssign(bool assign) {
    pos += 7;
    const char* revertPos = pos - 1;
    char ch = commentWhitespace();
    switch (ch) {
      case '.': {
        pos++;
        ch = commentWhitespace();
        const char* startPos = pos;
        if (identifier(ch)) {
          const char* endPos = pos;
          ch = commentWhitespace();
          if (ch == '=') {
            addExport(startPos, endPos);
            return;
          }
        }
        break;
      }
      case '[': {
        pos++;
        ch = commentWhitespace();
        if (ch == '\'' || ch == '"') {
          const char* startPos = pos;
          stringLiteral(ch);
          const char* endPos = ++pos;
          ch = commentWhitespace();
          if (ch != ']') break;
          pos++;
          ch = commentWhitespace();
          if (ch != '=') break;
          addExport(startPos, endPos);
        }
        break;
      }
      case '=': {
        if (assign) {
          clearReexports();
          pos++;
          ch = commentWhitespace();
          if (ch == '{') {
            tryParseLiteralExports();
            return;
          }
          if (ch == 'r')
            tryParseRequire(RequireType::ExportAssign);
        }
        break;
      }
    }
    pos = revertPos;
  }

  void tryParseModuleExportsDotAssign() {
    pos += 6;
    const char* revertPos = pos - 1;
    char ch = commentWhitespace();
    if (ch == '.') {
      pos++;
      ch = commentWhitespace();
      if (ch == 'e' && matchesAt(pos + 1, end, "xports")) {
        tryParseExportsDotAssign(true);
        return;
      }
    }
    pos = revertPos;
  }

  bool tryParseObjectHasOwnProperty(const char* it_id_start, size_t it_id_len) {
    char ch = commentWhitespace();
    if (ch != 'O' || !matchesAt(pos + 1, end, "bject")) return false;
    pos += 6;
    ch = commentWhitespace();
    if (ch != '.') return false;
    pos++;
    ch = commentWhitespace();
    if (ch == 'p') {
      if (!matchesAt(pos + 1, end, "rototype")) return false;
      pos += 9;
      ch = commentWhitespace();
      if (ch != '.') return false;
      pos++;
      ch = commentWhitespace();
    }
    if (ch != 'h' || !matchesAt(pos + 1, end, "asOwnProperty")) return false;
    pos += 14;
    ch = commentWhitespace();
    if (ch != '.') return false;
    pos++;
    ch = commentWhitespace();
    if (ch != 'c' || !matchesAt(pos + 1, end, "all")) return false;
    pos += 4;
    ch = commentWhitespace();
    if (ch != '(') return false;
    pos++;
    ch = commentWhitespace();
    if (!identifier(ch)) return false;
    ch = commentWhitespace();
    if (ch != ',') return false;
    pos++;
    ch = commentWhitespace();
    if (pos + it_id_len > end || memcmp(pos, it_id_start, it_id_len) != 0) return false;
    pos += it_id_len;
    ch = commentWhitespace();
    if (ch != ')') return false;
    pos++;
    return true;
  }

  void tryParseObjectDefineOrKeys(bool keys) {
    pos += 6;
    const char* revertPos = pos - 1;
    char ch = commentWhitespace();
    if (ch == '.') {
      pos++;
      ch = commentWhitespace();
      if (ch == 'd' && matchesAt(pos + 1, end, "efineProperty")) {
        const char* exportStart = nullptr;
        const char* exportEnd = nullptr;
        while (true) {
          pos += 14;
          revertPos = pos - 1;
          ch = commentWhitespace();
          if (ch != '(') break;
          pos++;
          ch = commentWhitespace();
          if (!readExportsOrModuleDotExports(ch)) break;
          ch = commentWhitespace();
          if (ch != ',') break;
          pos++;
          ch = commentWhitespace();
          if (ch != '\'' && ch != '"') break;
          exportStart = pos;
          stringLiteral(ch);
          exportEnd = ++pos;
          ch = commentWhitespace();
          if (ch != ',') break;
          pos++;
          ch = commentWhitespace();
          if (ch != '{') break;
          pos++;
          ch = commentWhitespace();
          if (ch == 'e') {
            if (!matchesAt(pos + 1, end, "numerable")) break;
            pos += 10;
            ch = commentWhitespace();
            if (ch != ':') break;
            pos++;
            ch = commentWhitespace();
            if (ch != 't' || !matchesAt(pos + 1, end, "rue")) break;
            pos += 4;
            ch = commentWhitespace();
            if (ch != ',') break;
            pos++;
            ch = commentWhitespace();
          }
          if (ch == 'v') {
            if (!matchesAt(pos + 1, end, "alue")) break;
            pos += 5;
            ch = commentWhitespace();
            if (ch != ':') break;
            if (exportStart && exportEnd)
              addExport(exportStart, exportEnd);
            pos = revertPos;
            return;
          } else if (ch == 'g') {
            if (!matchesAt(pos + 1, end, "et")) break;
            pos += 3;
            ch = commentWhitespace();
            if (ch == ':') {
              pos++;
              ch = commentWhitespace();
              if (ch != 'f') break;
              if (!matchesAt(pos + 1, end, "unction")) break;
              pos += 8;
              const char* lastPos = pos;
              ch = commentWhitespace();
              if (ch != '(' && (lastPos == pos || !identifier(ch))) break;
              ch = commentWhitespace();
            }
            if (ch != '(') break;
            pos++;
            ch = commentWhitespace();
            if (ch != ')') break;
            pos++;
            ch = commentWhitespace();
            if (ch != '{') break;
            pos++;
            ch = commentWhitespace();
            if (ch != 'r') break;
            if (!matchesAt(pos + 1, end, "eturn")) break;
            pos += 6;
            ch = commentWhitespace();
            if (!identifier(ch)) break;
            ch = commentWhitespace();
            if (ch == '.') {
              pos++;
              ch = commentWhitespace();
              if (!identifier(ch)) break;
              ch = commentWhitespace();
            } else if (ch == '[') {
              pos++;
              ch = commentWhitespace();
              if (ch == '\'' || ch == '"') {
                stringLiteral(ch);
              } else {
                break;
              }
              pos++;
              ch = commentWhitespace();
              if (ch != ']') break;
              pos++;
              ch = commentWhitespace();
            }
            if (ch == ';') {
              pos++;
              ch = commentWhitespace();
            }
            if (ch != '}') break;
            pos++;
            ch = commentWhitespace();
            if (ch == ',') {
              pos++;
              ch = commentWhitespace();
            }
            if (ch != '}') break;
            pos++;
            ch = commentWhitespace();
            if (ch != ')') break;
            if (exportStart && exportEnd)
              addExport(exportStart, exportEnd);
            return;
          }
          break;
        }
      } else if (keys && ch == 'k' && matchesAt(pos + 1, end, "eys")) {
        while (true) {
          pos += 4;
          revertPos = pos - 1;
          ch = commentWhitespace();
          if (ch != '(') break;
          pos++;
          ch = commentWhitespace();
          const char* id_start = pos;
          if (!identifier(ch)) break;
          size_t id_len = pos - id_start;
          ch = commentWhitespace();
          if (ch != ')') break;

          revertPos = pos++;
          ch = commentWhitespace();
          if (ch != '.') break;
          pos++;
          ch = commentWhitespace();
          if (ch != 'f' || !matchesAt(pos + 1, end, "orEach")) break;
          pos += 7;
          ch = commentWhitespace();
          revertPos = pos - 1;
          if (ch != '(') break;
          pos++;
          ch = commentWhitespace();
          if (ch != 'f' || !matchesAt(pos + 1, end, "unction")) break;
          pos += 8;
          ch = commentWhitespace();
          if (ch != '(') break;
          pos++;
          ch = commentWhitespace();
          const char* it_id_start = pos;
          if (!identifier(ch)) break;
          size_t it_id_len = pos - it_id_start;
          ch = commentWhitespace();
          if (ch != ')') break;
          pos++;
          ch = commentWhitespace();
          if (ch != '{') break;
          pos++;
          ch = commentWhitespace();
          if (ch != 'i' || *(pos + 1) != 'f') break;
          pos += 2;
          ch = commentWhitespace();
          if (ch != '(') break;
          pos++;
          ch = commentWhitespace();
          if (pos + it_id_len > end || memcmp(pos, it_id_start, it_id_len) != 0) break;
          pos += it_id_len;
          ch = commentWhitespace();

          if (ch == '=') {
            if (!matchesAt(pos + 1, end, "==")) break;
            pos += 3;
            ch = commentWhitespace();
            if (ch != '"' && ch != '\'') break;
            char quot = ch;
            if (!matchesAt(pos + 1, end, "default")) break;
            pos += 8;
            ch = commentWhitespace();
            if (ch != quot) break;
            pos++;
            ch = commentWhitespace();
            if (ch != '|' || *(pos + 1) != '|') break;
            pos += 2;
            ch = commentWhitespace();
            if (pos + it_id_len > end || memcmp(pos, it_id_start, it_id_len) != 0) break;
            pos += it_id_len;
            ch = commentWhitespace();
            if (ch != '=' || !matchesAt(pos + 1, end, "==")) break;
            pos += 3;
            ch = commentWhitespace();
            if (ch != '"' && ch != '\'') break;
            quot = ch;
            if (!matchesAt(pos + 1, end, "__esModule")) break;
            pos += 11;
            ch = commentWhitespace();
            if (ch != quot) break;
            pos++;
            ch = commentWhitespace();
            if (ch != ')') break;
            pos++;
            ch = commentWhitespace();
            if (ch != 'r' || !matchesAt(pos + 1, end, "eturn")) break;
            pos += 6;
            ch = commentWhitespace();
            if (ch == ';')
              pos++;
            ch = commentWhitespace();

            if (ch == 'i' && *(pos + 1) == 'f') {
              bool inIf = true;
              pos += 2;
              ch = commentWhitespace();
              if (ch != '(') break;
              pos++;
              const char* ifInnerPos = pos;

              if (tryParseObjectHasOwnProperty(it_id_start, it_id_len)) {
                ch = commentWhitespace();
                if (ch != ')') break;
                pos++;
                ch = commentWhitespace();
                if (ch != 'r' || !matchesAt(pos + 1, end, "eturn")) break;
                pos += 6;
                ch = commentWhitespace();
                if (ch == ';')
                  pos++;
                ch = commentWhitespace();
                if (ch == 'i' && *(pos + 1) == 'f') {
                  pos += 2;
                  ch = commentWhitespace();
                  if (ch != '(') break;
                  pos++;
                } else {
                  inIf = false;
                }
              } else {
                pos = ifInnerPos;
              }

              if (inIf) {
                if (pos + it_id_len > end || memcmp(pos, it_id_start, it_id_len) != 0) break;
                pos += it_id_len;
                ch = commentWhitespace();
                if (ch != 'i' || !matchesAt(pos + 1, end, "n ")) break;
                pos += 3;
                ch = commentWhitespace();
                if (!readExportsOrModuleDotExports(ch)) break;
                ch = commentWhitespace();
                if (ch != '&' || *(pos + 1) != '&') break;
                pos += 2;
                ch = commentWhitespace();
                if (!readExportsOrModuleDotExports(ch)) break;
                ch = commentWhitespace();
                if (ch != '[') break;
                pos++;
                ch = commentWhitespace();
                if (pos + it_id_len > end || memcmp(pos, it_id_start, it_id_len) != 0) break;
                pos += it_id_len;
                ch = commentWhitespace();
                if (ch != ']') break;
                pos++;
                ch = commentWhitespace();
                if (ch != '=' || !matchesAt(pos + 1, end, "==")) break;
                pos += 3;
                ch = commentWhitespace();
                if (pos + id_len > end || memcmp(pos, id_start, id_len) != 0) break;
                pos += id_len;
                ch = commentWhitespace();
                if (ch != '[') break;
                pos++;
                ch = commentWhitespace();
                if (pos + it_id_len > end || memcmp(pos, it_id_start, it_id_len) != 0) break;
                pos += it_id_len;
                ch = commentWhitespace();
                if (ch != ']') break;
                pos++;
                ch = commentWhitespace();
                if (ch != ')') break;
                pos++;
                ch = commentWhitespace();
                if (ch != 'r' || !matchesAt(pos + 1, end, "eturn")) break;
                pos += 6;
                ch = commentWhitespace();
                if (ch == ';')
                  pos++;
                ch = commentWhitespace();
              }
            }
          } else if (ch == '!') {
            if (!matchesAt(pos + 1, end, "==")) break;
            pos += 3;
            ch = commentWhitespace();
            if (ch != '"' && ch != '\'') break;
            char quot = ch;
            if (!matchesAt(pos + 1, end, "default")) break;
            pos += 8;
            ch = commentWhitespace();
            if (ch != quot) break;
            pos++;
            ch = commentWhitespace();
            if (ch == '&') {
              if (*(pos + 1) != '&') break;
              pos += 2;
              ch = commentWhitespace();
              if (ch != '!') break;
              pos++;
              ch = commentWhitespace();
              if (ch == 'O' && matchesAt(pos + 1, end, "bject.")) {
                if (!tryParseObjectHasOwnProperty(it_id_start, it_id_len)) break;
              } else if (identifier(ch)) {
                ch = commentWhitespace();
                if (ch != '.') break;
                pos++;
                ch = commentWhitespace();
                if (ch != 'h' || !matchesAt(pos + 1, end, "asOwnProperty")) break;
                pos += 14;
                ch = commentWhitespace();
                if (ch != '(') break;
                pos++;
                ch = commentWhitespace();
                if (pos + it_id_len > end || memcmp(pos, it_id_start, it_id_len) != 0) break;
                pos += it_id_len;
                ch = commentWhitespace();
                if (ch != ')') break;
                pos++;
              }
              ch = commentWhitespace();
            }
            if (ch != ')') break;
            pos++;
            ch = commentWhitespace();
          } else {
            break;
          }

          if (readExportsOrModuleDotExports(ch)) {
            ch = commentWhitespace();
            if (ch != '[') break;
            pos++;
            ch = commentWhitespace();
            if (pos + it_id_len > end || memcmp(pos, it_id_start, it_id_len) != 0) break;
            pos += it_id_len;
            ch = commentWhitespace();
            if (ch != ']') break;
            pos++;
            ch = commentWhitespace();
            if (ch != '=') break;
            pos++;
            ch = commentWhitespace();
            if (pos + id_len > end || memcmp(pos, id_start, id_len) != 0) break;
            pos += id_len;
            ch = commentWhitespace();
            if (ch != '[') break;
            pos++;
            ch = commentWhitespace();
            if (pos + it_id_len > end || memcmp(pos, it_id_start, it_id_len) != 0) break;
            pos += it_id_len;
            ch = commentWhitespace();
            if (ch != ']') break;
            pos++;
            ch = commentWhitespace();
            if (ch == ';') {
              pos++;
              ch = commentWhitespace();
            }
          } else if (ch == 'O') {
            if (!matchesAt(pos + 1, end, "bject")) break;
            pos += 6;
            ch = commentWhitespace();
            if (ch != '.') break;
            pos++;
            ch = commentWhitespace();
            if (ch != 'd' || !matchesAt(pos + 1, end, "efineProperty")) break;
            pos += 14;
            ch = commentWhitespace();
            if (ch != '(') break;
            pos++;
            ch = commentWhitespace();
            if (!readExportsOrModuleDotExports(ch)) break;
            ch = commentWhitespace();
            if (ch != ',') break;
            pos++;
            ch = commentWhitespace();
            if (pos + it_id_len > end || memcmp(pos, it_id_start, it_id_len) != 0) break;
            pos += it_id_len;
            ch = commentWhitespace();
            if (ch != ',') break;
            pos++;
            ch = commentWhitespace();
            if (ch != '{') break;
            pos++;
            ch = commentWhitespace();
            if (ch != 'e' || !matchesAt(pos + 1, end, "numerable")) break;
            pos += 10;
            ch = commentWhitespace();
            if (ch != ':') break;
            pos++;
            ch = commentWhitespace();
            if (ch != 't' || !matchesAt(pos + 1, end, "rue")) break;
            pos += 4;
            ch = commentWhitespace();
            if (ch != ',') break;
            pos++;
            ch = commentWhitespace();
            if (ch != 'g' || !matchesAt(pos + 1, end, "et")) break;
            pos += 3;
            ch = commentWhitespace();
            if (ch == ':') {
              pos++;
              ch = commentWhitespace();
              if (ch != 'f') break;
              if (!matchesAt(pos + 1, end, "unction")) break;
              pos += 8;
              const char* lastPos = pos;
              ch = commentWhitespace();
              if (ch != '(' && (lastPos == pos || !identifier(ch))) break;
              ch = commentWhitespace();
            }
            if (ch != '(') break;
            pos++;
            ch = commentWhitespace();
            if (ch != ')') break;
            pos++;
            ch = commentWhitespace();
            if (ch != '{') break;
            pos++;
            ch = commentWhitespace();
            if (ch != 'r' || !matchesAt(pos + 1, end, "eturn")) break;
            pos += 6;
            ch = commentWhitespace();
            if (pos + id_len > end || memcmp(pos, id_start, id_len) != 0) break;
            pos += id_len;
            ch = commentWhitespace();
            if (ch != '[') break;
            pos++;
            ch = commentWhitespace();
            if (pos + it_id_len > end || memcmp(pos, it_id_start, it_id_len) != 0) break;
            pos += it_id_len;
            ch = commentWhitespace();
            if (ch != ']') break;
            pos++;
            ch = commentWhitespace();
            if (ch == ';') {
              pos++;
              ch = commentWhitespace();
            }
            if (ch != '}') break;
            pos++;
            ch = commentWhitespace();
            if (ch == ',') {
              pos++;
              ch = commentWhitespace();
            }
            if (ch != '}') break;
            pos++;
            ch = commentWhitespace();
            if (ch != ')') break;
            pos++;
            ch = commentWhitespace();
            if (ch == ';') {
              pos++;
              ch = commentWhitespace();
            }
          } else {
            break;
          }

          if (ch != '}') break;
          pos++;
          ch = commentWhitespace();
          if (ch != ')') break;

          // Search through export bindings to see if this is a star export
          StarExportBinding* curCheckBinding = &starExportStack_[0];
          while (curCheckBinding != starExportStack) {
            if (id_len == static_cast<size_t>(curCheckBinding->id_end - curCheckBinding->id_start) &&
                memcmp(id_start, curCheckBinding->id_start, id_len) == 0) {
              addReexport(curCheckBinding->specifier_start, curCheckBinding->specifier_end);
              pos = revertPos;
              return;
            }
            curCheckBinding++;
          }
          return;
        }
      }
    }
    pos = revertPos;
  }

  void tryBacktrackAddStarExportBinding(const char* bPos) {
    while (*bPos == ' ' && bPos > source)
      bPos--;
    if (*bPos == '=') {
      bPos--;
      while (*bPos == ' ' && bPos > source)
        bPos--;
      const char* id_end = bPos;
      bool identifierStart = false;
      while (bPos > source) {
        char ch = *bPos;
        if (!isIdentifierChar(static_cast<uint8_t>(ch)))
          break;
        identifierStart = isIdentifierStart(static_cast<uint8_t>(ch));
        bPos--;
      }
      if (identifierStart && *bPos == ' ') {
        if (starExportStack == STAR_EXPORT_STACK_END)
          return;
        starExportStack->id_start = bPos + 1;
        starExportStack->id_end = id_end + 1;
        while (*bPos == ' ' && bPos > source)
          bPos--;
        switch (*bPos) {
          case 'r':
            if (!readPrecedingKeyword2(bPos - 1, "va"))
              return;
            break;
          case 't':
            if (!readPrecedingKeyword2(bPos - 1, "le") && !readPrecedingKeyword4(bPos - 1, "cons"))
              return;
            break;
          default:
            return;
        }
        starExportStack++;
      }
    }
  }

  void throwIfImportStatement() {
    const char* startPos = pos;
    pos += 6;
    char ch = commentWhitespace();
    switch (ch) {
      case '(':
        openTokenPosStack_[openTokenDepth++] = startPos;
        return;
      case '.':
        // Check if followed by 'meta' (possibly with whitespace)
        pos++;
        ch = commentWhitespace();
        // Use str_eq4 for more efficient comparison
        if (ch == 'm' && pos + 4 <= end && matchesAt(pos + 1, end, "eta")) {
          // Check that 'meta' is not followed by an identifier character
          if (pos + 4 < end && isIdentifierChar(static_cast<uint8_t>(pos[4]))) {
            // It's something like import.metaData, not import.meta
            return;
          }
          syntaxError(lexer_error::UNEXPECTED_ESM_IMPORT_META);
        }
        return;
      default:
        if (pos == startPos + 6)
          break;
        [[fallthrough]];
      case '"':
      case '\'':
      case '{':
      case '*':
        if (openTokenDepth != 0) {
          pos--;
          return;
        }
        syntaxError(lexer_error::UNEXPECTED_ESM_IMPORT);
    }
  }

  void throwIfExportStatement() {
    pos += 6;
    const char* curPos = pos;
    char ch = commentWhitespace();
    if (pos == curPos && !isPunctuator(ch))
      return;
    syntaxError(lexer_error::UNEXPECTED_ESM_EXPORT);
  }

public:
  CJSLexer() : source(nullptr), pos(nullptr), end(nullptr), lastTokenPos(nullptr),
               templateStackDepth(0), openTokenDepth(0), templateDepth(0), braceDepth(0),
               lastSlashWasDivision(false), nextBraceIsClass(false),
               starExportStack(nullptr), STAR_EXPORT_STACK_END(nullptr),
               exports(nullptr), re_exports(nullptr) {}

  bool parse(std::string_view file_contents, std::vector<std::string>& out_exports, std::vector<std::string>& out_re_exports) {
    source = file_contents.data();
    pos = source - 1;
    end = source + file_contents.size();
    // Initialize lastTokenPos to before source to detect start-of-input condition
    // when checking if '/' should be treated as regex vs division operator
    lastTokenPos = source - 1;

    exports = &out_exports;
    re_exports = &out_re_exports;

    templateStackDepth = 0;
    openTokenDepth = 0;
    templateDepth = std::numeric_limits<uint16_t>::max();
    lastSlashWasDivision = false;
    parse_error.reset();
    starExportStack = &starExportStack_[0];
    STAR_EXPORT_STACK_END = &starExportStack_[MAX_STAR_EXPORTS - 1];
    nextBraceIsClass = false;

    char ch = '\0';

    // Handle shebang
    if (file_contents.size() >= 2 && source[0] == '#' && source[1] == '!') {
      if (file_contents.size() == 2)
        return true;
      pos += 2;
      while (pos < end) {
        ch = *pos;
        if (ch == '\n' || ch == '\r')
          break;
        pos++;
      }
      lastTokenPos = pos;  // Update lastTokenPos after shebang
    }

    while (pos++ < end) {
      ch = *pos;

      if (ch == ' ' || (ch < 14 && ch > 8))
        continue;

      if (openTokenDepth == 0) {
        switch (ch) {
          case 'i':
            if (pos + 6 < end && matchesAt(pos + 1, end, "mport") && keywordStart(pos))
              throwIfImportStatement();
            lastTokenPos = pos;
            continue;
          case 'r': {
            const char* startPos = pos;
            if (tryParseRequire(RequireType::Import) && keywordStart(startPos))
              tryBacktrackAddStarExportBinding(startPos - 1);
            lastTokenPos = pos;
            continue;
          }
          case '_':
            if (pos + 23 < end && matchesAt(pos + 1, end, "interopRequireWildcard") && (keywordStart(pos) || *(pos - 1) == '.')) {
              const char* startPos = pos;
              pos += 23;
              if (*pos == '(') {
                pos++;
                openTokenPosStack_[openTokenDepth++] = lastTokenPos;
                if (tryParseRequire(RequireType::Import) && keywordStart(startPos))
                  tryBacktrackAddStarExportBinding(startPos - 1);
              }
            } else if (pos + 8 < end && matchesAt(pos + 1, end, "_export") && (keywordStart(pos) || *(pos - 1) == '.')) {
              pos += 8;
              if (pos + 4 < end && matchesAt(pos, end, "Star"))
                pos += 4;
              if (*pos == '(') {
                openTokenPosStack_[openTokenDepth++] = lastTokenPos;
                if (*(pos + 1) == 'r') {
                  pos++;
                  tryParseRequire(RequireType::ExportStar);
                }
              }
            }
            lastTokenPos = pos;
            continue;
        }
      }

      switch (ch) {
        case 'e':
          if (pos + 6 < end && matchesAt(pos + 1, end, "xport") && keywordStart(pos)) {
            if (pos + 7 < end && *(pos + 6) == 's')
              tryParseExportsDotAssign(false);
            else if (openTokenDepth == 0)
              throwIfExportStatement();
          }
          break;
        case 'c':
          if (keywordStart(pos) && matchesAt(pos + 1, end, "lass") && isBrOrWs(*(pos + 5)))
            nextBraceIsClass = true;
          break;
        case 'm':
          if (pos + 6 < end && matchesAt(pos + 1, end, "odule") && keywordStart(pos))
            tryParseModuleExportsDotAssign();
          break;
        case 'O':
          if (pos + 6 < end && matchesAt(pos + 1, end, "bject") && keywordStart(pos))
            tryParseObjectDefineOrKeys(openTokenDepth == 0);
          break;
        case '(':
          openTokenPosStack_[openTokenDepth++] = lastTokenPos;
          break;
        case ')':
          if (openTokenDepth == 0) {
            syntaxError(lexer_error::UNEXPECTED_PAREN);
            return false;
          }
          openTokenDepth--;
          break;
        case '{':
          openClassPosStack[openTokenDepth] = nextBraceIsClass;
          nextBraceIsClass = false;
          openTokenPosStack_[openTokenDepth++] = lastTokenPos;
          break;
        case '}':
          if (openTokenDepth == 0) {
            syntaxError(lexer_error::UNEXPECTED_BRACE);
            return false;
          }
          if (openTokenDepth-- == templateDepth) {
            templateDepth = templateStack_[--templateStackDepth];
            templateString();
          } else {
            if (templateDepth != std::numeric_limits<uint16_t>::max() && openTokenDepth < templateDepth) {
              syntaxError(lexer_error::UNTERMINATED_TEMPLATE_STRING);
              return false;
            }
          }
          break;
        case '\'':
        case '"':
          stringLiteral(ch);
          break;
        case '/': {
          char next_ch = pos + 1 < end ? *(pos + 1) : '\0';
          if (next_ch == '/') {
            lineComment();
            continue;
          } else if (next_ch == '*') {
            blockComment();
            continue;
          } else {
            // Check if lastTokenPos is before the source (start of input)
            bool isStartOfInput = lastTokenPos < source;
            char lastToken = isStartOfInput ? '\0' : *lastTokenPos;
            
            if ((isExpressionPunctuator(lastToken) &&
                 !(lastToken == '.' && lastTokenPos > source && *(lastTokenPos - 1) >= '0' && *(lastTokenPos - 1) <= '9') &&
                 !(lastToken == '+' && lastTokenPos > source && *(lastTokenPos - 1) == '+') &&
                 !(lastToken == '-' && lastTokenPos > source && *(lastTokenPos - 1) == '-')) ||
                (lastToken == ')' && isParenKeyword(openTokenPosStack_[openTokenDepth])) ||
                (lastToken == '}' && (openTokenPosStack_[openTokenDepth] < source || isExpressionTerminator(openTokenPosStack_[openTokenDepth]) || openClassPosStack[openTokenDepth])) ||
                (lastToken == '/' && lastSlashWasDivision) ||
                (!isStartOfInput && isExpressionKeyword(lastTokenPos)) ||
                !lastToken || isStartOfInput) {
              regularExpression();
              lastSlashWasDivision = false;
            } else {
              lastSlashWasDivision = true;
            }
          }
          break;
        }
        case '`':
          if (templateDepth == std::numeric_limits<uint16_t>::max() - 1) {
            syntaxError(lexer_error::TEMPLATE_NEST_OVERFLOW);
            return false;
          }
          templateString();
          break;
      }
      lastTokenPos = pos;
    }

    if (templateDepth != std::numeric_limits<uint16_t>::max() || openTokenDepth || parse_error) {
      return false;
    }

    return true;
  }

  std::optional<lexer_error> get_error() const {
    return parse_error;
  }
};

// Global state for error tracking
std::optional<lexer_error> last_error;

std::optional<lexer_analysis> parse_commonjs(const std::string_view file_contents) {
  last_error.reset();

  lexer_analysis result;
  CJSLexer lexer;

  if (lexer.parse(file_contents, result.exports, result.re_exports)) {
    return result;
  }

  last_error = lexer.get_error();
  return std::nullopt;
}

std::optional<lexer_error> get_last_error() {
  return last_error;
}

}  // namespace lexer

# commonjs-lexer

A fast C++ lexer for extracting named exports from CommonJS modules. This library performs static analysis to detect CommonJS export patterns without executing the code.

## Features

- **Fast**: Zero-copy parsing for most exports using `std::string_view`
- **Accurate**: Handles complex CommonJS patterns including re-exports, Object.defineProperty, and transpiler output
- **Unicode Support**: Properly unescapes JavaScript string literals including `\u{XXXX}` and surrogate pairs
- **No Dependencies**: Single-header distribution available
- **Cross-Platform**: Works on Linux, macOS, and Windows

## Installation

### CMake

```cmake
include(FetchContent)
FetchContent_Declare(
  commonjs-lexer
  GIT_REPOSITORY https://github.com/user/commonjs-lexer.git
  GIT_TAG main
)
FetchContent_MakeAvailable(commonjs-lexer)

target_link_libraries(your_target PRIVATE lexer::lexer)
```

### Single Header

Copy `singleheader/lexer.h` and `singleheader/lexer.cpp` to your project.

## Usage

```cpp
#include "lexer.h"
#include <iostream>

int main() {
  std::string_view source = R"(
    exports.foo = 1;
    exports.bar = function() {};
    module.exports.baz = 'hello';
  )";

  auto result = lexer::parse_commonjs(source);
  
  if (result) {
    std::cout << "Exports found:" << std::endl;
    for (const auto& exp : result->exports) {
      std::cout << "  - " << lexer::get_string_view(exp) << std::endl;
    }
  }
  
  return 0;
}
```

Output:
```
Exports found:
  - foo
  - bar
  - baz
```

## API Reference

### `lexer::parse_commonjs`

```cpp
std::optional<lexer_analysis> parse_commonjs(std::string_view file_contents);
```

Parses CommonJS source code and extracts export information.

**Parameters:**
- `file_contents`: The JavaScript source code to analyze

**Returns:**
- `std::optional<lexer_analysis>`: Analysis result, or `std::nullopt` on parse error

### `lexer::lexer_analysis`

```cpp
struct lexer_analysis {
  std::vector<export_string> exports;      // Named exports
  std::vector<export_string> re_exports;   // Re-exported module specifiers
};
```

### `lexer::export_string`

```cpp
using export_string = std::variant<std::string, std::string_view>;
```

Export names are stored as a variant to avoid unnecessary copies:
- `std::string_view`: Used for simple identifiers (zero-copy, points to source)
- `std::string`: Used when unescaping is needed (e.g., Unicode escapes)

### `lexer::get_string_view`

```cpp
inline std::string_view get_string_view(const export_string& s);
```

Helper function to get a `string_view` from either variant type.

### `lexer::get_last_error`

```cpp
const std::optional<lexer_error>& get_last_error();
```

Returns the last parse error, if any.

## Supported Patterns

### Direct Exports

```javascript
exports.foo = 1;
exports['bar'] = 2;
module.exports.baz = 3;
module.exports['qux'] = 4;
```

### Object Literal Assignment

```javascript
module.exports = {
  foo: 1,
  bar: someValue,
  'string-key': 3
};
```

### Object.defineProperty

```javascript
Object.defineProperty(exports, 'foo', { value: 1 });
Object.defineProperty(exports, 'bar', {
  enumerable: true,
  get: function() { return something; }
});
```

### Re-exports (Transpiler Patterns)

```javascript
// Babel/TypeScript __export pattern
function __export(m) {
  for (var p in m) if (!exports.hasOwnProperty(p)) exports[p] = m[p];
}
__export(require("./other-module"));

// Object.keys forEach pattern
Object.keys(_module).forEach(function(key) {
  if (key === "default" || key === "__esModule") return;
  exports[key] = _module[key];
});
```

### Spread Re-exports

```javascript
module.exports = {
  ...require('./other-module'),
  foo: 1
};
```

## Unicode Handling

The lexer properly handles JavaScript string escape sequences:

```javascript
exports['\u0066\u006f\u006f'] = 1;     // 'foo'
exports['\u{1F310}'] = 2;              // Globe emoji
exports['\u{D83C}\u{DF10}'] = 3;       // Surrogate pair for emoji
exports['caf\u00e9'] = 4;              // 'cafe' with accent
```

Invalid escape sequences (like lone surrogates) are filtered out.

## ESM Detection

The lexer detects ESM syntax and returns an error:

```javascript
import foo from 'bar';        // Error: UNEXPECTED_ESM_IMPORT
export const foo = 1;         // Error: UNEXPECTED_ESM_EXPORT
import.meta.url;              // Error: UNEXPECTED_ESM_IMPORT_META
```

This helps identify files that should be parsed as ES modules instead.

## Error Handling

```cpp
auto result = lexer::parse_commonjs(source);

if (!result) {
  auto error = lexer::get_last_error();
  if (error) {
    switch (*error) {
      case lexer::UNEXPECTED_ESM_IMPORT:
        std::cerr << "File contains ESM import syntax" << std::endl;
        break;
      case lexer::UNTERMINATED_STRING_LITERAL:
        std::cerr << "Unterminated string literal" << std::endl;
        break;
      // ... handle other errors
    }
  }
}
```

## Building

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

### Running Tests

```bash
cmake --build . --target real_world_tests
./tests/real_world_tests
```

### Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `LEXER_TESTING` | `ON` | Build test suite |
| `LEXER_BENCHMARKS` | `OFF` | Build benchmarks |

## Performance

The lexer is optimized for speed:

- Single-pass parsing with no backtracking
- Zero-copy for most export names using `std::string_view`
- String allocation only when unescaping is required
- Efficient character classification using lookup tables

## License

MIT License - see [LICENSE-MIT](LICENSE-MIT) for details.

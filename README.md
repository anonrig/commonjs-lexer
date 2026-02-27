# merve

A fast C++ lexer for extracting named exports from CommonJS modules. This library performs static analysis to detect CommonJS export patterns without executing the code.

## Features

- **Fast**: Zero-copy parsing for most exports using `std::string_view`
- **Accurate**: Handles complex CommonJS patterns including re-exports, Object.defineProperty, and transpiler output
- **Source Locations**: Each export includes a 1-based line number for tooling integration
- **Unicode Support**: Properly unescapes JavaScript string literals including `\u{XXXX}` and surrogate pairs
- **Optional SIMD Acceleration**: Can use [simdutf](https://github.com/simdutf/simdutf) for faster string operations
- **C API**: Full C interface (`merve_c.h`) for use from C, FFI, or other languages
- **No Dependencies**: Single-header distribution available (simdutf is optional)
- **Cross-Platform**: Works on Linux, macOS, and Windows

## Installation

### CMake

```cmake
include(FetchContent)
FetchContent_Declare(
  merve
  GIT_REPOSITORY https://github.com/anonrig/merve.git
  GIT_TAG main
)
FetchContent_MakeAvailable(merve)

target_link_libraries(your_target PRIVATE lexer::lexer)
```

### Single Header

Copy `singleheader/merve.h` and `singleheader/merve.cpp` to your project.
The C API header `singleheader/merve_c.h` is also included in the distribution.

## Usage

```cpp
#include "merve.h"
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
      std::cout << "  - " << lexer::get_string_view(exp)
                << " (line " << exp.line << ")" << std::endl;
    }
  }

  return 0;
}
```

Output:
```
Exports found:
  - foo (line 2)
  - bar (line 3)
  - baz (line 4)
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
  std::vector<export_entry> exports;      // Named exports
  std::vector<export_entry> re_exports;   // Re-exported module specifiers
};
```

### `lexer::export_entry`

```cpp
struct export_entry {
  export_string name;
  uint32_t line;  // 1-based line number
};
```

Each export/re-export entry includes the name and the 1-based line number where it was found in the source.

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
inline std::string_view get_string_view(const export_entry& e);
```

Helper function to get a `string_view` from an `export_string` or `export_entry`.

### `lexer::get_last_error`

```cpp
const std::optional<lexer_error>& get_last_error();
```

Returns the last parse error, if any.

### `lexer::get_last_error_location`

```cpp
const std::optional<error_location>& get_last_error_location();
```

Returns the location of the last parse error, if available. Location tracking
is enabled when built with `MERVE_ENABLE_ERROR_LOCATION`.

### `lexer::error_location`

```cpp
struct error_location {
  uint32_t line;    // 1-based
  uint32_t column;  // 1-based
  size_t offset;    // 0-based byte offset
};
```

## C API

merve provides a C API (`merve_c.h`) for use from C programs, FFI bindings, or any language that can call C functions. The C API is compiled into the merve library alongside the C++ implementation.

### C API Usage

```c
#include "merve_c.h"
#include <stdio.h>
#include <string.h>

int main(void) {
  const char* source = "exports.foo = 1;\nexports.bar = 2;\n";

  merve_error_loc err_loc = {0, 0, 0};
  merve_analysis result = merve_parse_commonjs_ex(
      source, strlen(source), &err_loc);

  if (merve_is_valid(result)) {
    size_t count = merve_get_exports_count(result);
    printf("Found %zu exports:\n", count);
    for (size_t i = 0; i < count; i++) {
      merve_string name = merve_get_export_name(result, i);
      uint32_t line = merve_get_export_line(result, i);
      printf("  - %.*s (line %u)\n", (int)name.length, name.data, line);
    }
  } else {
    printf("Parse error: %d\n", merve_get_last_error());
    if (err_loc.line != 0) {
      printf("  at line %u, column %u (byte offset %zu)\n",
             err_loc.line, err_loc.column, err_loc.offset);
    }
  }

  merve_free(result);
  return 0;
}
```

Output:
```
Found 2 exports:
  - foo (line 1)
  - bar (line 2)
```

### C API Reference

#### Types

| Type | Description |
|------|-------------|
| `merve_string` | Non-owning string reference (`data` + `length`). Not null-terminated. |
| `merve_analysis` | Opaque handle to a parse result. Must be freed with `merve_free()`. |
| `merve_version_components` | Struct with `major`, `minor`, `revision` fields. |
| `merve_error_loc` | Error location (`line`, `column`, `offset`). `{0,0,0}` means unavailable. |

#### Functions

| Function | Description |
|----------|-------------|
| `merve_parse_commonjs(input, length)` | Parse CommonJS source. Returns a handle (NULL only on OOM). |
| `merve_parse_commonjs_ex(input, length, out_err)` | Parse CommonJS source and optionally fill error location. |
| `merve_is_valid(result)` | Check if parsing succeeded. NULL-safe. |
| `merve_free(result)` | Free a parse result. NULL-safe. |
| `merve_get_exports_count(result)` | Number of named exports found. |
| `merve_get_reexports_count(result)` | Number of re-export specifiers found. |
| `merve_get_export_name(result, index)` | Get export name at index. Returns `{NULL, 0}` on error. |
| `merve_get_export_line(result, index)` | Get 1-based line number of export. Returns 0 on error. |
| `merve_get_reexport_name(result, index)` | Get re-export specifier at index. Returns `{NULL, 0}` on error. |
| `merve_get_reexport_line(result, index)` | Get 1-based line number of re-export. Returns 0 on error. |
| `merve_get_last_error()` | Last error code (`MERVE_ERROR_*`), or -1 if no error. |
| `merve_get_version()` | Version string (e.g. `"1.0.1"`). |
| `merve_get_version_components()` | Version as `{major, minor, revision}`. |

Build with `-DMERVE_ENABLE_ERROR_LOCATION=ON` to enable non-zero locations
from `merve_parse_commonjs_ex`.

#### Error Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `MERVE_ERROR_UNEXPECTED_ESM_IMPORT` | 10 | Found ESM `import` declaration |
| `MERVE_ERROR_UNEXPECTED_ESM_EXPORT` | 11 | Found ESM `export` declaration |
| `MERVE_ERROR_UNEXPECTED_ESM_IMPORT_META` | 9 | Found `import.meta` |
| `MERVE_ERROR_UNTERMINATED_STRING_LITERAL` | 6 | Unclosed string literal |
| `MERVE_ERROR_UNTERMINATED_TEMPLATE_STRING` | 5 | Unclosed template literal |
| `MERVE_ERROR_UNTERMINATED_REGEX` | 8 | Unclosed regular expression |
| `MERVE_ERROR_UNEXPECTED_PAREN` | 1 | Unexpected `)` |
| `MERVE_ERROR_UNEXPECTED_BRACE` | 2 | Unexpected `}` |
| `MERVE_ERROR_UNTERMINATED_PAREN` | 3 | Unclosed `(` |
| `MERVE_ERROR_UNTERMINATED_BRACE` | 4 | Unclosed `{` |
| `MERVE_ERROR_TEMPLATE_NEST_OVERFLOW` | 12 | Template literal nesting too deep |

#### Lifetime Rules

- The `merve_analysis` handle must be freed with `merve_free()`.
- `merve_string` values returned by accessors are valid as long as the handle has not been freed.
- For exports backed by a `string_view` (most identifiers), the original source buffer must also remain valid.
- All functions are NULL-safe: passing NULL returns safe defaults (false, 0, `{NULL, 0}`).

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
ctest --test-dir build
```

### Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `MERVE_TESTING` | `ON` | Build test suite |
| `MERVE_BENCHMARKS` | `OFF` | Build benchmarks |
| `MERVE_USE_SIMDUTF` | `OFF` | Use simdutf for optimized string operations |
| `MERVE_ENABLE_ERROR_LOCATION` | `OFF` | Track parse error source locations |
| `MERVE_SANITIZE` | `OFF` | Enable address sanitizer |

### Building with simdutf

To enable SIMD-accelerated string operations:

```bash
cmake -B build -DMERVE_USE_SIMDUTF=ON
cmake --build build
```

When `MERVE_USE_SIMDUTF=ON`, CMake will automatically fetch simdutf via CPM if it's not found on the system. The library uses simdutf's optimized `find()` function for faster escape sequence detection.

For projects that already have simdutf available (like Node.js), define `MERVE_USE_SIMDUTF=1` and ensure the simdutf header is in the include path.

## Performance

The lexer is optimized for speed:

- Single-pass parsing with no backtracking
- Zero-copy for most export names using `std::string_view`
- String allocation only when unescaping is required
- Compile-time lookup tables using C++20 `consteval`
- Optional SIMD acceleration via simdutf for escape sequence detection

## License

Licensed under either of

 * Apache License, Version 2.0
   ([LICENSE-APACHE](LICENSE-APACHE) or http://www.apache.org/licenses/LICENSE-2.0)
 * MIT license
   ([LICENSE-MIT](LICENSE-MIT) or http://opensource.org/licenses/MIT)

at your option.

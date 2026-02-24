# merve (Rust)

Fast CommonJS export lexer for Rust. Extracts named exports and re-exports from
CommonJS modules via static analysis, without executing the code.

This crate provides safe Rust bindings to the [merve](https://github.com/nodejs/merve) C++ library.

## Usage

Add to your `Cargo.toml`:

```toml
[dependencies]
merve = "0.1"
```

Parse CommonJS source and iterate over exports:

```rust
use merve::parse_commonjs;

let source = r#"
    exports.foo = 1;
    exports.bar = function() {};
    module.exports.baz = 'hello';
"#;

let analysis = parse_commonjs(source).expect("parse failed");

for export in analysis.exports() {
    println!("{} (line {})", export.name, export.line);
}
```

## Features

**std** (default): Enables `std::error::Error` impl for `LexerError`. Disable for `no_std`:

```toml
merve = { version = "0.1", default-features = false }
```

**libcpp**: Build the underlying C++ code with `libc++` instead of `libstdc++`.
Requires `libc++` to be installed:

```toml
merve = { version = "0.1", features = ["libcpp"] }
```

## API

### `parse_commonjs`

```rust
pub fn parse_commonjs(source: &str) -> Result<Analysis<'_>, LexerError>
```

Parse CommonJS source code and extract export information. The returned
`Analysis` borrows from `source` because export names may point directly into
the source buffer (zero-copy).

### `Analysis<'a>`

| Method | Returns | Description |
| -------- | --------- | ------------- |
| `exports_count()` | `usize` | Number of named exports |
| `reexports_count()` | `usize` | Number of re-export specifiers |
| `export_name(index)` | `Option<&'a str>` | Export name at index |
| `export_line(index)` | `Option<u32>` | 1-based line number of export |
| `reexport_name(index)` | `Option<&'a str>` | Re-export specifier at index |
| `reexport_line(index)` | `Option<u32>` | 1-based line number of re-export |
| `exports()` | `ExportIter` | Iterator over all exports |
| `reexports()` | `ExportIter` | Iterator over all re-exports |

### `Export<'a>`

```rust
pub struct Export<'a> {
    pub name: &'a str,
    pub line: u32,
}
```

### `LexerError`

Returned when the input contains ESM syntax or malformed constructs:

| Variant | Description |
| --------- | ------------- |
| `UnexpectedEsmImport` | Found `import` declaration |
| `UnexpectedEsmExport` | Found `export` declaration |
| `UnexpectedEsmImportMeta` | Found `import.meta` |
| `UnterminatedStringLiteral` | Unclosed string literal |
| `UnterminatedTemplateString` | Unclosed template literal |
| `UnterminatedRegex` | Unclosed regular expression |
| `UnexpectedParen` | Unexpected `)` |
| `UnexpectedBrace` | Unexpected `}` |
| `UnterminatedParen` | Unclosed `(` |
| `UnterminatedBrace` | Unclosed `{` |
| `TemplateNestOverflow` | Template literal nesting too deep |

`LexerError` implements `Display` and, with the `std` feature, `std::error::Error`.

### Versioning helpers

```rust
pub fn version() -> &'static str
pub fn version_components() -> (i32, i32, i32)
```

## Lifetime semantics

`Analysis<'a>` ties its lifetime to the source `&str` passed to `parse_commonjs`.
Export names returned by `export_name()` / the iterator borrow from the original
source buffer (the C++ library uses `std::string_view` for zero-copy export names).
Keep the source string alive as long as you access export names.

```rust
let source = String::from("exports.hello = 1;");
let analysis = merve::parse_commonjs(&source).unwrap();
// `analysis` borrows `source` -- both must stay alive
assert_eq!(analysis.export_name(0), Some("hello"));
```

## Thread safety

`Analysis` implements `Send` and `Sync`.

## License

Licensed under either of

* Apache License, Version 2.0
  ([LICENSE-APACHE](../LICENSE-APACHE) or <http://www.apache.org/licenses/LICENSE-2.0>)
* MIT license
  ([LICENSE-MIT](../LICENSE-MIT) or <http://opensource.org/licenses/MIT>)

at your option.

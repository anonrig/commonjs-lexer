//! # Merve
//!
//! Merve is a fast CommonJS export lexer written in C++.
//! This crate provides safe Rust bindings via the C API.
//!
//! ## Usage
//!
//! ```
//! use merve::parse_commonjs;
//!
//! let source = "exports.foo = 1; exports.bar = 2;";
//! let analysis = parse_commonjs(source).expect("parse failed");
//!
//! assert_eq!(analysis.exports_count(), 2);
//! for export in analysis.exports() {
//!     println!("{} (line {})", export.name, export.line);
//! }
//! ```
//!
//! ## no-std
//!
//! This crate supports `no_std` environments. Disable default features:
//!
//! ```toml
//! merve = { version = "0.1", default-features = false }
//! ```

#![cfg_attr(not(feature = "std"), no_std)]

mod ffi;

#[cfg(feature = "std")]
extern crate std;

use core::fmt;
use core::marker::PhantomData;

/// Error codes returned by the merve lexer.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum LexerError {
    EmptySource,
    UnexpectedParen,
    UnexpectedBrace,
    UnterminatedParen,
    UnterminatedBrace,
    UnterminatedTemplateString,
    UnterminatedStringLiteral,
    UnterminatedRegexCharacterClass,
    UnterminatedRegex,
    UnexpectedEsmImportMeta,
    UnexpectedEsmImport,
    UnexpectedEsmExport,
    TemplateNestOverflow,
    /// An error code not recognized by these bindings.
    Unknown(i32),
}

impl LexerError {
    /// Convert a C API error code to a `LexerError`.
    #[must_use]
    pub fn from_code(code: i32) -> Self {
        match code {
            0 => Self::EmptySource,
            1 => Self::UnexpectedParen,
            2 => Self::UnexpectedBrace,
            3 => Self::UnterminatedParen,
            4 => Self::UnterminatedBrace,
            5 => Self::UnterminatedTemplateString,
            6 => Self::UnterminatedStringLiteral,
            7 => Self::UnterminatedRegexCharacterClass,
            8 => Self::UnterminatedRegex,
            9 => Self::UnexpectedEsmImportMeta,
            10 => Self::UnexpectedEsmImport,
            11 => Self::UnexpectedEsmExport,
            12 => Self::TemplateNestOverflow,
            other => Self::Unknown(other),
        }
    }

    /// Return the short name of this error variant.
    #[must_use]
    pub fn as_str(&self) -> &'static str {
        match self {
            Self::EmptySource => "empty source",
            Self::UnexpectedParen => "unexpected parenthesis",
            Self::UnexpectedBrace => "unexpected brace",
            Self::UnterminatedParen => "unterminated parenthesis",
            Self::UnterminatedBrace => "unterminated brace",
            Self::UnterminatedTemplateString => "unterminated template string",
            Self::UnterminatedStringLiteral => "unterminated string literal",
            Self::UnterminatedRegexCharacterClass => "unterminated regex character class",
            Self::UnterminatedRegex => "unterminated regex",
            Self::UnexpectedEsmImportMeta => "unexpected ESM import.meta",
            Self::UnexpectedEsmImport => "unexpected ESM import",
            Self::UnexpectedEsmExport => "unexpected ESM export",
            Self::TemplateNestOverflow => "template nesting overflow",
            Self::Unknown(_) => "unknown error",
        }
    }
}

impl fmt::Display for LexerError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::Unknown(code) => write!(f, "merve lexer error: unknown (code {})", code),
            _ => write!(f, "merve lexer error: {}", self.as_str()),
        }
    }
}

#[cfg(feature = "std")]
impl std::error::Error for LexerError {}

/// A parsed CommonJS analysis result.
///
/// The lifetime `'a` is tied to the source string passed to [`parse_commonjs`],
/// because export names may reference slices of the original source buffer
/// (zero-copy `string_view` exports from the C++ side).
///
/// The handle is freed on drop.
pub struct Analysis<'a> {
    handle: ffi::merve_analysis,
    _source: PhantomData<&'a [u8]>,
}

impl<'a> Drop for Analysis<'a> {
    fn drop(&mut self) {
        unsafe { ffi::merve_free(self.handle) }
    }
}

// Safety: The C++ implementation does not use thread-local state in the
// analysis struct itself (`merve_get_last_error` is global, but `Analysis`
// does not rely on it after construction).
unsafe impl Send for Analysis<'_> {}
unsafe impl Sync for Analysis<'_> {}

impl<'a> Analysis<'a> {
    /// Number of named exports found.
    #[must_use]
    pub fn exports_count(&self) -> usize {
        unsafe { ffi::merve_get_exports_count(self.handle) }
    }

    /// Number of re-export module specifiers found.
    #[must_use]
    pub fn reexports_count(&self) -> usize {
        unsafe { ffi::merve_get_reexports_count(self.handle) }
    }

    /// Get the name of the export at `index`.
    ///
    /// Returns `None` if `index` is out of bounds.
    #[must_use]
    pub fn export_name(&self, index: usize) -> Option<&'a str> {
        if index >= self.exports_count() {
            return None;
        }
        let s = unsafe { ffi::merve_get_export_name(self.handle, index) };
        Some(unsafe { s.as_str() })
    }

    /// Get the 1-based source line number of the export at `index`.
    ///
    /// Returns `None` if `index` is out of bounds.
    #[must_use]
    pub fn export_line(&self, index: usize) -> Option<u32> {
        if index >= self.exports_count() {
            return None;
        }
        let line = unsafe { ffi::merve_get_export_line(self.handle, index) };
        if line == 0 { None } else { Some(line) }
    }

    /// Get the module specifier of the re-export at `index`.
    ///
    /// Returns `None` if `index` is out of bounds.
    #[must_use]
    pub fn reexport_name(&self, index: usize) -> Option<&'a str> {
        if index >= self.reexports_count() {
            return None;
        }
        let s = unsafe { ffi::merve_get_reexport_name(self.handle, index) };
        Some(unsafe { s.as_str() })
    }

    /// Get the 1-based source line number of the re-export at `index`.
    ///
    /// Returns `None` if `index` is out of bounds.
    #[must_use]
    pub fn reexport_line(&self, index: usize) -> Option<u32> {
        if index >= self.reexports_count() {
            return None;
        }
        let line = unsafe { ffi::merve_get_reexport_line(self.handle, index) };
        if line == 0 { None } else { Some(line) }
    }

    /// Iterate over all named exports.
    #[must_use]
    pub fn exports(&self) -> ExportIter<'a, '_> {
        ExportIter {
            analysis: self,
            kind: ExportKind::Export,
            index: 0,
            count: self.exports_count(),
        }
    }

    /// Iterate over all re-exports.
    #[must_use]
    pub fn reexports(&self) -> ExportIter<'a, '_> {
        ExportIter {
            analysis: self,
            kind: ExportKind::ReExport,
            index: 0,
            count: self.reexports_count(),
        }
    }
}

impl fmt::Debug for Analysis<'_> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("Analysis")
            .field("exports_count", &self.exports_count())
            .field("reexports_count", &self.reexports_count())
            .finish()
    }
}

/// A single export entry: a name and its source line number.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Export<'a> {
    /// The export name (or module specifier for re-exports).
    pub name: &'a str,
    /// 1-based source line number.
    pub line: u32,
}

impl fmt::Display for Export<'_> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{} (line {})", self.name, self.line)
    }
}

/// Distinguishes between exports and re-exports in [`ExportIter`].
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
enum ExportKind {
    Export,
    ReExport,
}

/// Iterator over exports or re-exports.
///
/// Created by [`Analysis::exports`] or [`Analysis::reexports`].
pub struct ExportIter<'a, 'b> {
    analysis: &'b Analysis<'a>,
    kind: ExportKind,
    index: usize,
    count: usize,
}

impl<'a> Iterator for ExportIter<'a, '_> {
    type Item = Export<'a>;

    fn next(&mut self) -> Option<Self::Item> {
        if self.index >= self.count {
            return None;
        }
        let i = self.index;
        self.index += 1;
        let (name, line) = match self.kind {
            ExportKind::Export => (
                self.analysis.export_name(i).unwrap_or(""),
                self.analysis.export_line(i).unwrap_or(0),
            ),
            ExportKind::ReExport => (
                self.analysis.reexport_name(i).unwrap_or(""),
                self.analysis.reexport_line(i).unwrap_or(0),
            ),
        };
        Some(Export { name, line })
    }

    fn size_hint(&self) -> (usize, Option<usize>) {
        let remaining = self.count - self.index;
        (remaining, Some(remaining))
    }
}

impl ExactSizeIterator for ExportIter<'_, '_> {}

/// Parse CommonJS source code and extract export information.
///
/// The returned [`Analysis`] borrows from `source` because some export names
/// may point directly into the source buffer (zero-copy `string_view` exports).
///
/// # Errors
///
/// Returns a [`LexerError`] if the input contains ESM syntax or other
/// unsupported constructs.
///
/// # Examples
///
/// ```
/// use merve::parse_commonjs;
///
/// let source = "exports.hello = 1;";
/// let analysis = parse_commonjs(source).unwrap();
/// assert_eq!(analysis.exports_count(), 1);
/// assert_eq!(analysis.export_name(0), Some("hello"));
/// ```
pub fn parse_commonjs(source: &str) -> Result<Analysis<'_>, LexerError> {
    if source.is_empty() {
        return Err(LexerError::EmptySource);
    }
    let handle = unsafe { ffi::merve_parse_commonjs(source.as_ptr().cast(), source.len()) };
    if handle.is_null() {
        // NULL means allocation failure; map to a generic error
        let code = unsafe { ffi::merve_get_last_error() };
        return Err(if code >= 0 {
            LexerError::from_code(code)
        } else {
            LexerError::Unknown(code)
        });
    }
    if !unsafe { ffi::merve_is_valid(handle) } {
        let code = unsafe { ffi::merve_get_last_error() };
        let err = if code >= 0 {
            LexerError::from_code(code)
        } else {
            LexerError::Unknown(code)
        };
        unsafe { ffi::merve_free(handle) };
        return Err(err);
    }
    Ok(Analysis {
        handle,
        _source: PhantomData,
    })
}

/// Get the merve library version string (e.g. `"1.0.1"`).
#[must_use]
pub fn version() -> &'static str {
    unsafe {
        let ptr = ffi::merve_get_version();
        let len = {
            let mut n = 0usize;
            while *ptr.add(n) != 0 {
                n += 1;
            }
            n
        };
        let slice = core::slice::from_raw_parts(ptr.cast(), len);
        core::str::from_utf8_unchecked(slice)
    }
}

/// Get the merve library version as `(major, minor, revision)`.
#[must_use]
pub fn version_components() -> (i32, i32, i32) {
    let v = unsafe { ffi::merve_get_version_components() };
    (v.major, v.minor, v.revision)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn version_is_not_empty() {
        let v = version();
        assert!(!v.is_empty());
        assert!(v.contains('.'), "version should contain a dot: {v}");
    }

    #[test]
    fn version_components_are_nonnegative() {
        let (major, minor, rev) = version_components();
        assert!(major >= 0);
        assert!(minor >= 0);
        assert!(rev >= 0);
    }

    #[test]
    fn parse_simple_exports() {
        let source = "exports.foo = 1; exports.bar = 2;";
        let analysis = parse_commonjs(source).expect("should parse");
        assert_eq!(analysis.exports_count(), 2);
        assert_eq!(analysis.export_name(0), Some("foo"));
        assert_eq!(analysis.export_name(1), Some("bar"));
        assert_eq!(analysis.reexports_count(), 0);
    }

    #[cfg(feature = "std")]
    #[test]
    fn parse_module_exports() {
        let source = "module.exports = { a, b, c };";
        let analysis = parse_commonjs(source).expect("should parse");
        assert_eq!(analysis.exports_count(), 3);
        assert_eq!(analysis.export_name(0), Some("a"));
        assert_eq!(analysis.export_name(1), Some("b"));
        assert_eq!(analysis.export_name(2), Some("c"));
    }

    #[test]
    fn parse_reexports() {
        let source = r#"module.exports = require("./other");"#;
        let analysis = parse_commonjs(source).expect("should parse");
        assert_eq!(analysis.reexports_count(), 1);
        assert_eq!(analysis.reexport_name(0), Some("./other"));
    }

    #[test]
    fn esm_import_returns_error() {
        let source = "import { foo } from 'bar';";
        let result = parse_commonjs(source);
        assert!(result.is_err());
        let err = result.unwrap_err();
        assert_eq!(err, LexerError::UnexpectedEsmImport);
    }

    #[test]
    fn esm_export_returns_error() {
        let source = "export const x = 1;";
        let result = parse_commonjs(source);
        assert!(result.is_err());
        let err = result.unwrap_err();
        assert_eq!(err, LexerError::UnexpectedEsmExport);
    }

    #[test]
    fn empty_input() {
        let result = parse_commonjs("");
        assert!(result.is_err());
        assert_eq!(result.unwrap_err(), LexerError::EmptySource);
    }

    #[test]
    fn out_of_bounds_returns_none() {
        let source = "exports.x = 1;";
        let analysis = parse_commonjs(source).expect("should parse");
        assert_eq!(analysis.export_name(999), None);
        assert_eq!(analysis.export_line(999), None);
        assert_eq!(analysis.reexport_name(0), None);
        assert_eq!(analysis.reexport_line(0), None);
    }

    #[test]
    fn export_lines() {
        let source = "exports.a = 1;\nexports.b = 2;\nexports.c = 3;";
        let analysis = parse_commonjs(source).expect("should parse");
        assert_eq!(analysis.export_line(0), Some(1));
        assert_eq!(analysis.export_line(1), Some(2));
        assert_eq!(analysis.export_line(2), Some(3));
    }

    #[cfg(feature = "std")]
    #[test]
    fn exports_iterator() {
        let source = "exports.x = 1; exports.y = 2;";
        let analysis = parse_commonjs(source).expect("should parse");
        let exports: Vec<Export<'_>> = analysis.exports().collect();
        assert_eq!(exports.len(), 2);
        assert_eq!(exports[0].name, "x");
        assert_eq!(exports[1].name, "y");
    }

    #[test]
    fn exports_iterator_exact_size() {
        let source = "exports.a = 1; exports.b = 2; exports.c = 3;";
        let analysis = parse_commonjs(source).expect("should parse");
        let iter = analysis.exports();
        assert_eq!(iter.len(), 3);
    }

    #[cfg(feature = "std")]
    #[test]
    fn reexports_iterator() {
        let source = r#"module.exports = require("./a");"#;
        let analysis = parse_commonjs(source).expect("should parse");
        let reexports: Vec<Export<'_>> = analysis.reexports().collect();
        assert_eq!(reexports.len(), 1);
        assert_eq!(reexports[0].name, "./a");
    }

    #[cfg(feature = "std")]
    #[test]
    fn debug_impl() {
        let source = "exports.z = 1;";
        let analysis = parse_commonjs(source).expect("should parse");
        let dbg = format!("{:?}", analysis);
        assert!(dbg.contains("Analysis"));
        assert!(dbg.contains("exports_count: 1"));
    }

    #[cfg(feature = "std")]
    #[test]
    fn export_display_impl() {
        let e = Export {
            name: "foo",
            line: 42,
        };
        assert_eq!(format!("{e}"), "foo (line 42)");
    }

    #[cfg(feature = "std")]
    #[test]
    fn error_display() {
        let err = LexerError::UnexpectedEsmImport;
        let s = format!("{err}");
        assert!(s.contains("unexpected ESM import"), "got: {s}");
    }

    #[cfg(feature = "std")]
    #[test]
    fn error_display_unknown() {
        let err = LexerError::Unknown(99);
        let s = format!("{err}");
        assert!(s.contains("99"), "got: {s}");
    }

    #[test]
    fn error_from_code_roundtrip() {
        for code in 0..=12 {
            let err = LexerError::from_code(code);
            assert_ne!(err, LexerError::Unknown(code));
        }
        assert_eq!(LexerError::from_code(999), LexerError::Unknown(999));
    }

    #[cfg(feature = "std")]
    #[test]
    fn error_is_std_error() {
        fn assert_error<E: std::error::Error>() {}
        assert_error::<LexerError>();
    }

    #[test]
    fn bracket_notation_exports() {
        let source = r#"exports["hello-world"] = 1;"#;
        let analysis = parse_commonjs(source).expect("should parse");
        assert_eq!(analysis.exports_count(), 1);
        assert_eq!(analysis.export_name(0), Some("hello-world"));
    }

    #[test]
    fn multiple_independent_parses() {
        let src1 = "exports.a = 1;";
        let src2 = "exports.b = 2;";
        let a1 = parse_commonjs(src1).expect("should parse");
        let a2 = parse_commonjs(src2).expect("should parse");
        assert_eq!(a1.export_name(0), Some("a"));
        assert_eq!(a2.export_name(0), Some("b"));
    }

    #[test]
    fn send_and_sync() {
        fn assert_send<T: Send>() {}
        fn assert_sync<T: Sync>() {}
        assert_send::<Analysis<'_>>();
        assert_sync::<Analysis<'_>>();
    }
}

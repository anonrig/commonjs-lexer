#![allow(non_camel_case_types)]
use core::ffi::{c_char, c_int, c_void};

/// Non-owning string reference returned by the C API.
///
/// The `data` pointer is NOT null-terminated. Always use `length`.
///
/// Valid as long as:
/// - The `merve_analysis` handle that produced it has not been freed.
/// - For `string_view`-backed exports: the original source buffer is alive.
#[repr(C)]
pub struct merve_string {
    pub data: *const c_char,
    pub length: usize,
}

impl merve_string {
    /// Convert to a Rust `&str` with an arbitrary lifetime.
    ///
    /// Returns `""` when `length` is 0 (which includes the case where `data` is null).
    ///
    /// # Safety
    /// The caller must ensure that the backing data outlives `'a` and is valid UTF-8.
    /// The `merve_string` itself is a temporary POD value; the data it points to
    /// lives in the original source buffer or the analysis handle.
    #[must_use]
    pub unsafe fn as_str<'a>(&self) -> &'a str {
        if self.length == 0 {
            return "";
        }
        unsafe {
            let slice = core::slice::from_raw_parts(self.data.cast(), self.length);
            core::str::from_utf8_unchecked(slice)
        }
    }
}

/// Opaque handle to a CommonJS parse result.
pub type merve_analysis = *mut c_void;

/// Version number components.
#[repr(C)]
pub struct merve_version_components {
    pub major: c_int,
    pub minor: c_int,
    pub revision: c_int,
}

unsafe extern "C" {
    pub fn merve_parse_commonjs(input: *const c_char, length: usize) -> merve_analysis;
    pub fn merve_is_valid(result: merve_analysis) -> bool;
    pub fn merve_free(result: merve_analysis);
    pub fn merve_get_exports_count(result: merve_analysis) -> usize;
    pub fn merve_get_reexports_count(result: merve_analysis) -> usize;
    pub fn merve_get_export_name(result: merve_analysis, index: usize) -> merve_string;
    pub fn merve_get_export_line(result: merve_analysis, index: usize) -> u32;
    pub fn merve_get_reexport_name(result: merve_analysis, index: usize) -> merve_string;
    pub fn merve_get_reexport_line(result: merve_analysis, index: usize) -> u32;
    pub fn merve_get_last_error() -> c_int;
    pub fn merve_get_version() -> *const c_char;
    pub fn merve_get_version_components() -> merve_version_components;
}

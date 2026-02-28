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

/// Opaque handle to a CommonJS parse result.
pub type merve_analysis = *mut c_void;

/// Version number components.
#[repr(C)]
pub struct merve_version_components {
    pub major: c_int,
    pub minor: c_int,
    pub revision: c_int,
}

#[cfg(feature = "error-location")]
#[repr(C)]
pub struct merve_error_loc {
    pub line: u32,
    pub column: u32,
}

unsafe extern "C" {
    pub fn merve_parse_commonjs(input: *const c_char, length: usize) -> merve_analysis;
    #[cfg(feature = "error-location")]
    pub fn merve_parse_commonjs_ex(
        input: *const c_char,
        length: usize,
        out_err: *mut merve_error_loc,
    ) -> merve_analysis;
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

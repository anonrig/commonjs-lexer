use regex::Regex;
use std::fmt::{Display, Formatter};
use std::fs::{self, File};
use std::io::Read;
use std::path::{Path, PathBuf};
use std::{env, fmt};

#[derive(Clone, Debug)]
pub struct Target {
    pub architecture: String,
    pub vendor: String,
    pub system: Option<String>,
    pub abi: Option<String>,
}

impl Target {
    pub fn as_strs(&self) -> (&str, &str, Option<&str>, Option<&str>) {
        (
            self.architecture.as_str(),
            self.vendor.as_str(),
            self.system.as_deref(),
            self.abi.as_deref(),
        )
    }
}

impl Display for Target {
    fn fmt(&self, f: &mut Formatter) -> fmt::Result {
        write!(f, "{}-{}", &self.architecture, &self.vendor)?;

        if let Some(ref system) = self.system {
            write!(f, "-{}", system)
        } else {
            Ok(())
        }?;

        if let Some(ref abi) = self.abi {
            write!(f, "-{}", abi)
        } else {
            Ok(())
        }
    }
}

pub fn ndk() -> String {
    env::var("ANDROID_NDK").expect("ANDROID_NDK variable not set")
}

pub fn target_arch(arch: &str) -> &str {
    match arch {
        "armv7" => "arm",
        "aarch64" => "arm64",
        "i686" => "x86",
        arch => arch,
    }
}

fn host_tag() -> &'static str {
    if cfg!(target_os = "windows") {
        "windows-x86_64"
    } else if cfg!(target_os = "linux") {
        "linux-x86_64"
    } else if cfg!(target_os = "macos") {
        "darwin-x86_64"
    } else {
        panic!("host os is not supported")
    }
}

/// Get NDK major version from source.properties
fn ndk_major_version(ndk_dir: &Path) -> u32 {
    let re = Regex::new(r"Pkg.Revision = (\d+)\.(\d+)\.(\d+)").unwrap();
    let mut source_properties =
        File::open(ndk_dir.join("source.properties")).expect("Couldn't open source.properties");
    let mut buf = String::new();
    source_properties
        .read_to_string(&mut buf)
        .expect("Could not read source.properties");
    let captures = re
        .captures(&buf)
        .expect("source.properties did not match the regex");
    captures[1].parse().expect("could not parse major version")
}

/// Recursively inline `#include "..."` directives, deduplicating by file name.
fn amalgamate_file(
    include_path: &Path,
    source_path: &Path,
    base_path: &Path,
    filename: &str,
    out: &mut String,
    included: &mut Vec<String>,
) {
    let file_path = base_path.join(filename);
    let content = fs::read_to_string(&file_path)
        .unwrap_or_else(|e| panic!("failed to read {}: {e}", file_path.display()));

    let include_re = Regex::new(r#"^\s*#\s*include\s+"([^"]+)""#).unwrap();

    out.push_str(&format!("/* begin file {filename} */\n"));

    for line in content.lines() {
        if let Some(caps) = include_re.captures(line) {
            let inc_file = caps[1].to_string();

            if included.contains(&inc_file) {
                continue;
            }

            let resolved = if include_path.join(&inc_file).exists() {
                included.push(inc_file.clone());
                Some((include_path.to_path_buf(), inc_file))
            } else if source_path.join(&inc_file).exists() {
                included.push(inc_file.clone());
                Some((source_path.to_path_buf(), inc_file))
            } else {
                // System or unrecognized include — keep as-is.
                None
            };

            if let Some((base, name)) = resolved {
                amalgamate_file(include_path, source_path, &base, &name, out, included);
            } else {
                out.push_str(line);
                out.push('\n');
            }
        } else {
            out.push_str(line);
            out.push('\n');
        }
    }

    out.push_str(&format!("/* end file {filename} */\n"));
}

/// When building inside the merve repository, produce the three amalgamated
/// files in `deps/`:  merve.h, merve.cpp, merve_c.h.
fn amalgamate_from_repo(project_root: &Path, deps: &Path) {
    let include_path = project_root.join("include");
    let source_path = project_root.join("src");

    // Remove stale files / subdirectories from a previous layout.
    if deps.exists() {
        fs::remove_dir_all(deps).ok();
    }
    fs::create_dir_all(deps).expect("failed to create deps/");

    let mut included: Vec<String> = Vec::new();

    // 1. Amalgamate merve.h (inlines merve/parser.h -> merve/version.h).
    let mut header = String::new();
    amalgamate_file(
        &include_path,
        &source_path,
        &include_path,
        "merve.h",
        &mut header,
        &mut included,
    );
    fs::write(deps.join("merve.h"), &header).expect("failed to write deps/merve.h");

    // 2. Amalgamate merve.cpp (parser.cpp + merve_c.cpp with includes resolved).
    let mut source = String::from("#include \"merve.h\"\n\n");
    for cpp in &["parser.cpp", "merve_c.cpp"] {
        amalgamate_file(
            &include_path,
            &source_path,
            &source_path,
            cpp,
            &mut source,
            &mut included,
        );
    }
    fs::write(deps.join("merve.cpp"), &source).expect("failed to write deps/merve.cpp");

    // 3. Copy merve_c.h verbatim (standalone C header).
    fs::copy(include_path.join("merve_c.h"), deps.join("merve_c.h"))
        .expect("failed to copy merve_c.h");
}

fn main() {
    let manifest_dir = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap());
    let deps = manifest_dir.join("deps");

    // Detect in-repo build by checking for the parent C++ sources.
    let project_root = manifest_dir.parent().unwrap();
    let in_repo = project_root.join("src/parser.cpp").exists()
        && project_root.join("src/merve_c.cpp").exists()
        && project_root.join("include/merve.h").exists();

    if in_repo {
        amalgamate_from_repo(project_root, &deps);

        // Rebuild when upstream C++ sources change.
        for src in &[
            "src/parser.cpp",
            "src/merve_c.cpp",
            "include/merve.h",
            "include/merve_c.h",
            "include/merve/parser.h",
            "include/merve/version.h",
        ] {
            println!(
                "cargo:rerun-if-changed={}",
                project_root.join(src).display()
            );
        }
    }

    // Both in-repo and published crate use the same layout: merve.cpp + merve.h + merve_c.h
    assert!(
        deps.join("merve.cpp").exists(),
        "No C++ sources found in deps/. \
         When building outside the repository, deps/ must contain the amalgamated sources."
    );

    let mut build = cc::Build::new();
    build.file(deps.join("merve.cpp"));
    build.include(&deps);
    build.cpp(true).std("c++20").warnings(false);

    if env::var_os("CARGO_FEATURE_ERROR_LOCATION").is_some() {
        build.define("MERVE_ENABLE_ERROR_LOCATION", Some("1"));
    }

    // Target handling
    let target_str = env::var("TARGET").unwrap();
    let target: Vec<String> = target_str.split('-').map(|s| s.into()).collect();
    assert!(target.len() >= 2, "Failed to parse TARGET {}", target_str);

    let abi = if target.len() > 3 {
        Some(target[3].clone())
    } else {
        None
    };
    let system = if target.len() > 2 {
        Some(target[2].clone())
    } else {
        None
    };
    let target = Target {
        architecture: target[0].clone(),
        vendor: target[1].clone(),
        system,
        abi,
    };

    let compile_target_arch = env::var("CARGO_CFG_TARGET_ARCH").expect("CARGO_CFG_TARGET_ARCH");
    let compile_target_os = env::var("CARGO_CFG_TARGET_OS").expect("CARGO_CFG_TARGET_OS");
    let compile_target_feature = env::var("CARGO_CFG_TARGET_FEATURE");

    match target.system.as_deref() {
        Some("android" | "androideabi") => {
            let ndk = ndk();
            let major = ndk_major_version(Path::new(&ndk));
            if major < 22 {
                build
                    .flag(format!("--sysroot={}/sysroot", ndk))
                    .flag(format!(
                        "-isystem{}/sources/cxx-stl/llvm-libc++/include",
                        ndk
                    ));
            } else {
                let host_toolchain = format!("{}/toolchains/llvm/prebuilt/{}", ndk, host_tag());
                build.flag(format!("--sysroot={}/sysroot", host_toolchain));
            }
        }
        _ => {
            if compile_target_arch.starts_with("wasm") && compile_target_os != "emscripten" {
                let wasi_sdk = env::var("WASI_SDK").unwrap_or_else(|_| "/opt/wasi-sdk".to_owned());
                assert!(
                    Path::new(&wasi_sdk).exists(),
                    "WASI SDK not found at {wasi_sdk}"
                );
                build.compiler(format!("{wasi_sdk}/bin/clang++"));
                let wasi_sysroot_lib = match compile_target_feature {
                    Ok(compile_target_feature) if compile_target_feature.contains("atomics") => {
                        "wasm32-wasip1-threads"
                    }
                    _ => "wasm32-wasip1",
                };
                println!(
                    "cargo:rustc-link-search={wasi_sdk}/share/wasi-sysroot/lib/{wasi_sysroot_lib}"
                );
                build.flag("-fno-exceptions");
                build.cpp_set_stdlib("c++");
                println!("cargo:rustc-link-lib=c++abi");
                if compile_target_os == "unknown" {
                    build.target("wasm32-wasip1");
                    println!("cargo:rustc-link-lib=c");
                    build.file(manifest_dir.join("wasi_to_unknown.cpp"));
                }
            }

            let compiler = build.get_compiler();
            if compiler.is_like_msvc() {
                build.static_crt(true);
                link_args::windows! {
                    unsafe {
                        no_default_lib(
                            "libcmt.lib",
                        );
                    }
                }
            } else if compiler.is_like_clang() && cfg!(feature = "libcpp") {
                build.cpp_set_stdlib("c++");
            }
        }
    }

    build.compile("merve");
}

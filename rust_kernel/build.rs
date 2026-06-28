extern crate bindgen;
extern crate cbindgen;

use std::env;
use std::path::PathBuf;

fn main() {
    let crate_dir = env::var("CARGO_MANIFEST_DIR").unwrap();
    let out_dir = PathBuf::from(env::var("OUT_DIR").unwrap());

    // 1. C-to-Rust (Same as before)
    bindgen::Builder::default()
        .header("../include/pmm.h")
        .header("../include/cpu.h")
        .header("../include/task.h")
        .use_core() 
        .ctypes_prefix("core::ffi")
        .clang_arg("-target")
        .clang_arg("i686-unknown-none-elf")
        .generate()
        .expect("Unable to generate bindings")
        .write_to_file(out_dir.join("c_bindings.rs"))
        .expect("Couldn't write bindings!");

    // 2. Rust-to-C (FREESTANDING CONFIGURATION)
    let mut config = cbindgen::Config::default();
    config.language = cbindgen::Language::C;
    config.no_includes = true; // Turn off all default includes (like stdlib.h)
    // Only include headers that are valid in freestanding i686-elf-gcc
    config.sys_includes = vec![
        "stdint.h".to_string(), 
        "stdbool.h".to_string(), 
        "stddef.h".to_string()
    ];

    cbindgen::Builder::new()
        .with_crate(crate_dir)
        .with_config(config)
        .generate()
        .expect("Unable to generate C bindings")
        .write_to_file("../include/rust_interface.h");
}

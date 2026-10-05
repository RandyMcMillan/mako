use std::env;
use std::fs;
use std::path::PathBuf;

fn main() {
    let manifest_dir = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap());
    let sys_dir = manifest_dir.join("sys");
    let sys_deps = sys_dir.join("deps").join("lcdb");
    let include_dir = sys_dir.join("include");

    println!("cargo:rerun-if-changed={}", include_dir.display());

    println!("cargo:rustc-link-search=native={}", sys_dir.display());
    println!("cargo:rustc-link-search=native={}", sys_deps.display());

    let mut headers = Vec::new();
    collect_headers(&include_dir, &include_dir, &mut headers);
    headers.sort();
    headers.dedup();

    let mut wrapper = String::new();
    for header in &headers {
        wrapper.push_str(&format!("#include \"{}\"\n", header));
    }

    let bindings = bindgen::Builder::default()
        .header_contents("wrapper.h", &wrapper)
        .clang_arg(format!("-I{}", include_dir.display()))
        .allowlist_type("^btc_.*")
        .allowlist_type("^kh_.*")
        .allowlist_type("^_json_value$")
        .allowlist_function("^btc_.*")
        .allowlist_var("^btc_.*")
        .generate_comments(true)
        .layout_tests(false)
        .generate()
        .expect("generate Rust FFI bindings");

    let out_dir = PathBuf::from(env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out_dir.join("bindings.rs"))
        .expect("write Rust FFI bindings");

    // These archives are already produced by the checked-in C build. Keep the
    // order broad enough to satisfy the static archive dependencies.
    for lib in ["client", "wallet", "node", "base", "io", "mako", "lcdb"] {
        println!("cargo:rustc-link-lib=static={lib}");
    }

    if env::var("CARGO_CFG_TARGET_FAMILY").map(|v| v == "unix").unwrap_or(false) {
        println!("cargo:rustc-link-lib=pthread");
        println!("cargo:rustc-link-lib=m");
    }

    if env::var("CARGO_CFG_TARGET_OS").map(|v| v == "linux").unwrap_or(false) {
        println!("cargo:rustc-link-lib=rt");
        println!("cargo:rustc-link-lib=dl");
    }

    if env::var("CARGO_CFG_TARGET_OS").map(|v| v == "windows").unwrap_or(false) {
        println!("cargo:rustc-link-lib=ws2_32");
        println!("cargo:rustc-link-lib=advapi32");
        println!("cargo:rustc-link-lib=kernel32");
    }
}

fn collect_headers(root: &PathBuf, dir: &PathBuf, out: &mut Vec<String>) {
    for entry in fs::read_dir(dir).expect("read include directory") {
        let entry = entry.expect("read include entry");
        let path = entry.path();

        if path.is_dir() {
            collect_headers(root, &path, out);
            continue;
        }

        if path.extension().and_then(|ext| ext.to_str()) != Some("h") {
            continue;
        }

        let rel = path
            .strip_prefix(root)
            .expect("strip include prefix")
            .to_string_lossy()
            .replace('\\', "/");
        out.push(rel);
    }
}

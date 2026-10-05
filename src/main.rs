use std::env;
use std::ffi::CStr;
use std::process::ExitCode;

use mako::network;

fn main() -> ExitCode {
    let mut args = env::args().skip(1);
    match args.next().as_deref() {
        None | Some("-h") | Some("--help") | Some("help") => {
            print_help();
            ExitCode::SUCCESS
        }
        Some("networks") => {
            print_networks();
            ExitCode::SUCCESS
        }
        Some("version") | Some("--version") | Some("-V") => {
            println!("{}", env!("CARGO_PKG_VERSION"));
            ExitCode::SUCCESS
        }
        Some(cmd) => {
            eprintln!("unknown command: {cmd}");
            print_help();
            ExitCode::from(2)
        }
    }
}

fn print_help() {
    println!("mako-rs {}", env!("CARGO_PKG_VERSION"));
    println!("Rust FFI wrapper for the C mako libraries in ./sys");
    println!();
    println!("Usage:");
    println!("  mako-rs [command]");
    println!();
    println!("Commands:");
    println!("  help       Show this help");
    println!("  networks   Print the built-in mako network names");
    println!("  version    Print the Rust crate version");
    println!();
    println!("Options:");
    println!("  -h, --help Show this help");
    println!("  -V, --version Print the Rust crate version");
    println!();
    println!("Examples:");
    println!("  cargo run --bin mako-rs -- help");
    println!("  cargo run --bin mako-rs -- networks");
    println!("  cargo run --bin mako-rs -- version");
}

fn print_networks() {
    for (label, network) in [
        ("mainnet", network::mainnet()),
        ("testnet", network::testnet()),
        ("regtest", network::regtest()),
        ("simnet", network::simnet()),
        ("signet", network::signet()),
    ] {
        let name = unsafe {
            let name_ptr = (*network).name;
            CStr::from_ptr(name_ptr)
        };
        println!("{label}: {}", name.to_string_lossy());
    }
}

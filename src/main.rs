#![allow(unsafe_op_in_unsafe_fn)]

use std::env;
use std::ffi::{CStr, CString};
use std::os::raw::c_void;
use std::process::ExitCode;

use mako::ffi;
use mako::network;

fn main() -> ExitCode {
    let args: Vec<String> = env::args().collect();
    match args.get(1).map(|s| s.as_str()) {
        Some("help") => {
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
        Some(cmd) if !cmd.starts_with('-') => {
            eprintln!("unknown command: {cmd}");
            print_help();
            ExitCode::from(2)
        }
        _ => unsafe {
            match run_node(&args) {
                Ok(()) => ExitCode::SUCCESS,
                Err(code) => ExitCode::from(code),
            }
        }
    }
}

fn print_help() {
    println!("mako-rs {}", env!("CARGO_PKG_VERSION"));
    println!("Rust entrypoint for the mako bitcoin node via the C FFI in ./sys");
    println!();
    println!("Usage:");
    println!("  mako-rs [options]");
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
    println!("  cargo run --bin mako-rs --");
    println!("  cargo run --bin mako-rs -- -testnet");
    println!("  cargo run --bin mako-rs -- help");
    println!("  cargo run --bin mako-rs -- networks");
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

unsafe fn run_node(args: &[String]) -> Result<(), u8> {
    let mut argv_c = Vec::with_capacity(args.len());
    for arg in args {
        argv_c.push(CString::new(arg.as_str()).map_err(|_| 2u8)?);
    }

    let mut argv: Vec<*mut i8> = argv_c
        .iter_mut()
        .map(|arg| arg.as_ptr() as *mut i8)
        .collect();

    const PATH_MAX: usize = 1024;

    let prefix = {
        let mut buf = [0i8; PATH_MAX];
        let datadir = CString::new("mako").map_err(|_| 2u8)?;

        if ffi::btc_sys_datadir(buf.as_mut_ptr(), buf.len(), datadir.as_ptr()) == 0 {
            eprintln!("Could not find suitable datadir.");
            return Err(1);
        }

        if ffi::btc_path_absolutify(buf.as_mut_ptr(), buf.len()) == 0 {
            eprintln!("Path for datadir is too long!");
            return Err(1);
        }

        buf
    };

    let conf = ffi::btc_conf_create(argv.len() as i32, argv.as_mut_ptr(), prefix.as_ptr(), 0);
    if conf.is_null() {
        return Err(1);
    }

    if (*conf).help != 0 {
        print_node_help();
        ffi::btc_conf_destroy(conf);
        return Ok(());
    }

    if (*conf).version != 0 {
        println!("{}", env!("CARGO_PKG_VERSION"));
        ffi::btc_conf_destroy(conf);
        return Ok(());
    }

    if (*conf).daemon != 0 && ffi::btc_ps_daemon() == 0 {
        eprintln!("Could not daemonize process.");
        ffi::btc_conf_destroy(conf);
        return Err(1);
    }

    ffi::btc_net_startup();

    let node = ffi::btc_node_create((*conf).network);
    if node.is_null() {
        ffi::btc_net_cleanup();
        ffi::btc_conf_destroy(conf);
        return Err(1);
    }

    apply_config(node, conf);

    if ffi::btc_node_open(node, (*conf).prefix.as_ptr(), get_node_flags(conf)) == 0 {
        ffi::btc_node_destroy(node);
        ffi::btc_net_cleanup();
        ffi::btc_conf_destroy(conf);
        return Err(1);
    }

    ffi::btc_ps_onterm(Some(on_sigterm), node.cast::<c_void>());
    ffi::btc_node_start(node);

    ffi::btc_node_close(node);
    ffi::btc_node_destroy(node);
    ffi::btc_net_cleanup();
    ffi::btc_conf_destroy(conf);

    Ok(())
}

fn print_node_help() {
    println!("Usage: mako-rs [options]");
    println!("  -?, -h, --help   Show this help message");
    println!("  -version         Print the version");
    println!("  -testnet         Run on testnet");
    println!("  -daemon          Run in the background");
    println!("  -rpcport=<port>  Set RPC port");
    println!("  -port=<port>     Set P2P port");
}

unsafe fn apply_config(node: *mut ffi::btc_node_t, conf: *const ffi::btc_conf_t) {
    ffi::btc_logger_set_level((*node).logger, (*conf).level as _);

    ffi::btc_chain_set_threads((*node).chain, (*conf).workers);
    ffi::btc_chain_set_cache((*node).chain, ((*conf).cache_size as usize) << 20);

    ffi::btc_pool_set_port((*node).pool, (*conf).port);

    for i in 0..(*conf).bind.length {
        let addr = *(*conf).bind.items.add(i) as *mut ffi::btc_netaddr_t;
        ffi::btc_pool_set_bind((*node).pool, addr);
    }

    for i in 0..(*conf).external.length {
        let addr = *(*conf).external.items.add(i) as *mut ffi::btc_netaddr_t;
        ffi::btc_pool_set_external((*node).pool, addr);
    }

    for i in 0..(*conf).connect.length {
        let addr = *(*conf).connect.items.add(i) as *mut ffi::btc_netaddr_t;
        ffi::btc_pool_set_connect((*node).pool, addr);
    }

    ffi::btc_pool_set_proxy((*node).pool, &(*conf).proxy);
    ffi::btc_pool_set_maxinbound((*node).pool, (*conf).max_inbound as usize);
    ffi::btc_pool_set_maxoutbound((*node).pool, (*conf).max_outbound as usize);
    ffi::btc_pool_set_bantime((*node).pool, (*conf).ban_time as i64);
    ffi::btc_pool_set_onlynet((*node).pool, (*conf).only_net);

    ffi::btc_rpc_set_port((*node).rpc, (*conf).rpc_port);

    for i in 0..(*conf).rpc_bind.length {
        let addr = *(*conf).rpc_bind.items.add(i) as *mut ffi::btc_netaddr_t;
        ffi::btc_rpc_set_bind((*node).rpc, addr);
    }

    ffi::btc_rpc_set_credentials(
        (*node).rpc,
        (*conf).rpc_user.as_ptr(),
        (*conf).rpc_pass.as_ptr(),
    );
}

unsafe fn get_node_flags(conf: *const ffi::btc_conf_t) -> ffi::btc_node_flags {
    let mut flags: ffi::btc_node_flags = 0;

    if (*conf).checkpoints != 0 {
        flags |= ffi::btc_node_flags_BTC_CHAIN_CHECKPOINTS;
    }

    if (*conf).prune != 0 {
        flags |= ffi::btc_node_flags_BTC_CHAIN_PRUNE;
    }

    if (*conf).listen != 0 {
        flags |= ffi::btc_node_flags_BTC_POOL_LISTEN;
    }

    if (*conf).checkpoints != 0 {
        flags |= ffi::btc_node_flags_BTC_POOL_CHECKPOINTS;
    }

    if (*conf).connect.length > 0 || (*conf).no_connect != 0 {
        flags |= ffi::btc_node_flags_BTC_POOL_CONNECT;
    }

    if ffi::btc_netaddr_is_null(&(*conf).proxy) == 0 {
        flags |= ffi::btc_node_flags_BTC_POOL_PROXY;
    }

    if (*conf).discover != 0 {
        flags |= ffi::btc_node_flags_BTC_POOL_DISCOVER;
    }

    if (*conf).upnp != 0 {
        flags |= ffi::btc_node_flags_BTC_POOL_UPNP;
    }

    if (*conf).onion != 0 {
        flags |= ffi::btc_node_flags_BTC_POOL_ONION;
    }

    if (*conf).blocks_only != 0 {
        flags |= ffi::btc_node_flags_BTC_POOL_BLOCKSONLY;
    }

    if (*conf).bip37 != 0 {
        flags |= ffi::btc_node_flags_BTC_POOL_BIP37;
    }

    if (*conf).bip152 != 0 {
        flags |= ffi::btc_node_flags_BTC_POOL_BIP152;
    }

    if (*conf).bip157 != 0 {
        flags |= ffi::btc_node_flags_BTC_POOL_BIP157;
    }

    flags
}

unsafe extern "C" fn on_sigterm(arg: *mut c_void) {
    unsafe { ffi::btc_node_stop(arg as *mut ffi::btc_node_t) };
}

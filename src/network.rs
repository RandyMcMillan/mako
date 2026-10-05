use crate::ffi::{
    btc_checkpoint_t, btc_deployment_t, btc_mainnet, btc_network_bip30, btc_network_checkpoint,
    btc_network_deployment, btc_network_t, btc_regtest, btc_signet, btc_simnet, btc_testnet,
};
use std::ffi::CStr;

pub fn mainnet() -> *const btc_network_t {
    unsafe { btc_mainnet }
}

pub fn testnet() -> *const btc_network_t {
    unsafe { btc_testnet }
}

pub fn regtest() -> *const btc_network_t {
    unsafe { btc_regtest }
}

pub fn simnet() -> *const btc_network_t {
    unsafe { btc_simnet }
}

pub fn signet() -> *const btc_network_t {
    unsafe { btc_signet }
}

pub unsafe fn checkpoint(network: *const btc_network_t, height: i32) -> *const btc_checkpoint_t {
    unsafe { btc_network_checkpoint(network, height) }
}

pub unsafe fn bip30(network: *const btc_network_t, height: i32) -> *const btc_checkpoint_t {
    unsafe { btc_network_bip30(network, height) }
}

pub unsafe fn deployment(network: *const btc_network_t, name: &CStr) -> *const btc_deployment_t {
    unsafe { btc_network_deployment(network, name.as_ptr()) }
}

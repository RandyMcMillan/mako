//! Rust FFI bindings for the C implementation in `sys/`.
//!
//! The crate exposes the raw C ABI plus a few thin wrappers around the most
//! commonly used handle types.

pub mod client;
pub mod ffi;
pub mod logger;
pub mod network;

pub use client::Client;
pub use logger::Logger;

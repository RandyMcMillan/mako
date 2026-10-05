use crate::ffi::{btc_client_auth, btc_client_call, btc_client_close, btc_client_create, btc_client_destroy, btc_client_open, btc_client_t, btc_json_t};
use core::ptr::NonNull;
use std::ffi::CStr;

pub struct Client {
    raw: NonNull<btc_client_t>,
}

impl Client {
    pub fn new() -> Option<Self> {
        NonNull::new(unsafe { btc_client_create() }).map(|raw| Self { raw })
    }

    pub fn as_ptr(&self) -> *mut btc_client_t {
        self.raw.as_ptr()
    }

    pub fn open(&mut self, hostname: &CStr, port: i32, family: i32) -> i32 {
        unsafe { btc_client_open(self.as_ptr(), hostname.as_ptr(), port, family) }
    }

    pub fn auth(&mut self, user: &CStr, pass: &CStr) {
        unsafe { btc_client_auth(self.as_ptr(), user.as_ptr(), pass.as_ptr()) }
    }

    pub fn call(&mut self, method: &CStr, params: *mut btc_json_t) -> *mut btc_json_t {
        unsafe { btc_client_call(self.as_ptr(), method.as_ptr(), params) }
    }

    pub fn close(&mut self) {
        unsafe { btc_client_close(self.as_ptr()) }
    }
}

impl Drop for Client {
    fn drop(&mut self) {
        unsafe { btc_client_destroy(self.as_ptr()) }
    }
}

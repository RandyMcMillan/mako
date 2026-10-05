use crate::ffi::{btc_loglevel, btc_logger_close, btc_logger_create, btc_logger_destroy, btc_logger_open, btc_logger_set_level, btc_logger_set_silent, btc_logger_t};
use core::ptr::NonNull;
use std::ffi::CStr;

pub struct Logger {
    raw: NonNull<btc_logger_t>,
}

impl Logger {
    pub fn new() -> Option<Self> {
        NonNull::new(unsafe { btc_logger_create() }).map(|raw| Self { raw })
    }

    pub fn as_ptr(&self) -> *mut btc_logger_t {
        self.raw.as_ptr()
    }

    pub fn set_level(&mut self, level: btc_loglevel) {
        unsafe { btc_logger_set_level(self.as_ptr(), level) }
    }

    pub fn set_silent(&mut self, silent: bool) {
        unsafe { btc_logger_set_silent(self.as_ptr(), i32::from(silent)) }
    }

    pub fn open(&mut self, file: &CStr) -> i32 {
        unsafe { btc_logger_open(self.as_ptr(), file.as_ptr()) }
    }

    pub fn close(&mut self) {
        unsafe { btc_logger_close(self.as_ptr()) }
    }
}

impl Drop for Logger {
    fn drop(&mut self) {
        unsafe { btc_logger_destroy(self.as_ptr()) }
    }
}

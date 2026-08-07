use std::pin::Pin;

// C++ Bridge
#[cxx::bridge]
pub mod ffi {

    unsafe extern "C++" {
        include!("mylib.hpp");

        type RustMap;

        fn insert(self: Pin<&mut RustMap>, key: &str, value: String);
        fn rm(self: Pin<&mut RustMap>, key: &str) -> bool;
        fn get(self: &RustMap, key: &str) -> String;

        fn get_sections(value: String) -> UniquePtr<RustMap>;
    }
}

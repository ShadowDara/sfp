// C++ Bridge
#[cxx::bridge]
pub mod ffi {
    unsafe extern "C++" {
        include!("mylib.hpp");

        fn hello_cpp() -> String;
    }
}

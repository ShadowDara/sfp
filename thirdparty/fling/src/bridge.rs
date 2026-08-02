#[cxx::bridge]
pub mod ffi {
    unsafe extern "C++" {
        include!("rustffi/ffi.hpp");

        pub fn run_file(filename: &str);
    }

    unsafe extern "C++" {
        include!("rustffi/ffi.hpp");

        pub fn runREPL();
    }
}

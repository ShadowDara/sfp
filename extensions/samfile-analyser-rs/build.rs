fn main() {

    let mut bridge = cxx_build::bridge("src/main.rs");

    bridge
        .include("cpp/include")
        .compile("samfile-lsp");

    let dst = cmake::Config::new("cpp")
        .profile("Debug")
        .define(
            "CXXBRIDGE_INCLUDE",
            std::env::var("OUT_DIR").unwrap() + "/cxxbridge/include"
        )
        .build();


    println!("cargo:rustc-link-search=native={}/lib", dst.display());
    println!("cargo:rustc-link-lib=static=mylib");
}

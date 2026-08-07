fn main() {

    let mut bridge = cxx_build::bridge("src/bridge.rs");

    bridge
        .include("cpp/include")

        .include("cpp/libs/batch2")
        .include("cpp/libs/macroparser/include")
        .include("cpp/libs/sfp/include")
        .include("cpp/libs/sfp_lib/include")

        .include("cpp/thirdparty/fling")
        .include("cpp/thirdparty/kvp/include")

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

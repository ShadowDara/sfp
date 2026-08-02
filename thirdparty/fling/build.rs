// Changes
//
// Removed the dependency on dcorelib from the C++
// code in the flingr crate. This is because dcorelib
// is not needed for the functionality provided
// by the C++ code, and removing it simplifies the build
// process and reduces potential issues with linking
// against dcorelib.
//

fn main() {
    cxx_build::bridge("src/bridge.rs")
        .file("cpp/util.cpp")
        .file("cpp/rustffi/ffi.cpp")
        .file("cpp/frontend/lexer.cpp")
        .file("cpp/frontend/parser.cpp")
        .file("cpp/frontend/ast.cpp")
        .file("cpp/runtime/interpreter.cpp")
        .file("cpp/runtime/envirments.cpp")
        .file("cpp/runtime/eval/statements.cpp")
        .file("cpp/runtime/eval/expressions.cpp")
        .include("cpp")
        .flag_if_supported("/std:c++20")   // MSVC
        .flag_if_supported("-std=c++20")   // GCC/Clang fallback
        .compile("flingr");

    println!("cargo:rerun-if-changed=src/bridge.rs");
    println!("cargo:rerun-if-changed=cpp/util.cpp");
    println!("cargo:rerun-if-changed=cpp/util.hpp");
    println!("cargo:rerun-if-changed=cpp/rustffi/ffi.cpp");
    println!("cargo:rerun-if-changed=cpp/rustffi/ffi.hpp");
    println!("cargo:rerun-if-changed=cpp/frontend/lexer.cpp");
    println!("cargo:rerun-if-changed=cpp/frontend/lexer.hpp");
    println!("cargo:rerun-if-changed=cpp/frontend/parser.cpp");
    println!("cargo:rerun-if-changed=cpp/frontend/parser.hpp");
    println!("cargo:rerun-if-changed=cpp/frontend/ast.hpp");
    println!("cargo:rerun-if-changed=cpp/frontend/ast.cpp");
    println!("cargo:rerun-if-changed=cpp/runtime/interpreter.cpp");
    println!("cargo:rerun-if-changed=cpp/runtime/interpreter.hpp");
    println!("cargo:rerun-if-changed=cpp/runtime/envirments.cpp");
    println!("cargo:rerun-if-changed=cpp/runtime/envirments.hpp");
    println!("cargo:rerun-if-changed=cpp/runtime/values.hpp");
    println!("cargo:rerun-if-changed=cpp/runtime/eval/statements.cpp");
    println!("cargo:rerun-if-changed=cpp/runtime/eval/statements.hpp");
    println!("cargo:rerun-if-changed=cpp/runtime/eval/expressions.cpp");
    println!("cargo:rerun-if-changed=cpp/runtime/eval/expressions.hpp");
}

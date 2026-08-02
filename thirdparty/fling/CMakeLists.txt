cmake_minimum_required(VERSION 3.16)

set(CMAKE_COLOR_MAKEFILE ON)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

if (MSVC)
  add_compile_options(/utf-8)
  add_compile_options(/arch:AVX2)
  add_compile_options(/Gy)
  add_link_options(/OPT:REF)
endif()

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)

project("fling-lang" LANGUAGES CXX)

set(FLING_SOURCES
    cpp/frontend/ast.cpp
    cpp/frontend/lexer.cpp
    cpp/frontend/parser.cpp

    cpp/runtime/interpreter.cpp
    cpp/runtime/envirments.cpp
    cpp/runtime/eval/statements.cpp
    cpp/runtime/eval/expressions.cpp

    cpp/util.cpp
)

# Library erstellen
add_library(fling STATIC
    ${FLING_SOURCES}

    cpp/frontend/ast.hpp
    cpp/frontend/lexer.hpp
    cpp/frontend/parser.hpp
    cpp/runtime/values.hpp
    cpp/runtime/interpreter.hpp
)

target_include_directories(fling PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/cpp
)

set_target_properties(fling PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)


# Hauptprogramm
add_executable(fling-lang
    main.cpp
)

target_link_libraries(fling-lang PRIVATE fling)

set_target_properties(fling-lang PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

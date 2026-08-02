# CMakeList.txt: CMake-Projekt für "samfileparser++". Schließen Sie die Quelle ein, und definieren Sie
# projektspezifische Logik hier.
#
cmake_minimum_required (VERSION 3.8)

# Aktivieren Sie Hot Reload für MSVC-Compiler, sofern unterstützt.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE) #link time optimization


# for clang d
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


if(MSVC)
  add_compile_options(/arch:AVX2) #make sure SIMD optimizations take place
endif()


project(sfp)

set(VERSION_HEADER ${CMAKE_BINARY_DIR}/src/version.hpp)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/src/version.hpp
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/generate_version_header.py
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    DEPENDS ${CMAKE_SOURCE_DIR}/generate_version_header.py
)

add_custom_target(
    generate_version_header
    DEPENDS ${CMAKE_BINARY_DIR}/src/version.hpp
)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")


add_executable("${CMAKE_PROJECT_NAME}" "src/sfpmain.cpp" "src/sfp.rc" ${VERSION_HEADER})


add_dependencies(
    "${CMAKE_PROJECT_NAME}"
    generate_version_header
)

add_subdirectory(libs/sfp)
add_subdirectory(thirdparty/fling)
add_subdirectory(libs/macroparser)
add_subdirectory(thirdparty/kvp)
add_subdirectory(libs/batch2)

target_link_libraries("${CMAKE_PROJECT_NAME}" PRIVATE
    samfileparserlib
    fling
    macroparserlib
    batch2lib
    kvp
)

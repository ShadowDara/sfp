cmake_minimum_required(VERSION 3.15)

project(mylib)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")

add_subdirectory(thirdparty/fling)
add_subdirectory(thirdparty/kvp)

add_subdirectory(libs/sfp)
add_subdirectory(libs/macroparser)
add_subdirectory(libs/batch2)
add_subdirectory(libs/sfp_lib)

add_library(mylib STATIC
    src/mylib.cpp
)

target_include_directories(mylib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CXXBRIDGE_INCLUDE}
)

target_link_libraries("${CMAKE_PROJECT_NAME}" PRIVATE
    samfileparserlib
    fling
    macroparserlib
    batch2lib
    kvp
    sfp_lib
)

install(
    TARGETS mylib
    ARCHIVE DESTINATION lib
)

install(
    DIRECTORY include/
    DESTINATION include
)

# Wichtig für MSVC + Rust Debug
if(MSVC)
    target_compile_options(mylib PRIVATE
        /MD
    )

    target_compile_definitions(mylib PRIVATE
        _ITERATOR_DEBUG_LEVEL=0
    )
endif()

cmake_minimum_required(VERSION 3.16)

project(MeinProjekt
    VERSION 1.0
    LANGUAGES CXX
)

# C++ Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Library bauen
add_library(macroparserlib
    src/macroparser.cpp
)

# Header öffentlich machen
target_include_directories(macroparserlib
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

# Test-Executable
add_executable(mylib_test
    main.cpp
)

# Library mit Testprogramm linken
target_link_libraries(mylib_test
    PRIVATE
        macroparserlib
)

# Optional: Warnungen aktivieren
if (MSVC)
    target_compile_options(macroparserlib PRIVATE /W4)
    target_compile_options(mylib_test PRIVATE /W4)
else()
    target_compile_options(macroparserlib PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(mylib_test PRIVATE -Wall -Wextra -Wpedantic)
endif()

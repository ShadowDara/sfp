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


project(sfp)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_executable("${CMAKE_PROJECT_NAME}" "src/sfpmain.cpp" "src/sfp.rc")

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

# TODO: Fügen Sie bei Bedarf Tests hinzu, und installieren Sie Ziele.

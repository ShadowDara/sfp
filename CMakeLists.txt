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

project ("samfileparser++")

add_library(samfileparserlib)

target_include_directories(samfileparserlib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/")
target_sources(samfileparserlib PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/src/samfileparser++.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/parser.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/preprocessor.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/string_utils.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/runner.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/error.cpp"
)

set_property(TARGET samfileparserlib PROPERTY CXX_STANDARD 20)


# Fügen Sie der ausführbaren Datei dieses Projekts eine Quelle hinzu.
add_executable (sfp "${CMAKE_CURRENT_SOURCE_DIR}/src/exe/main.cpp")

target_link_libraries(sfp samfileparserlib)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET sfp PROPERTY CXX_STANDARD 20)
endif()

# TODO: Fügen Sie bei Bedarf Tests hinzu, und installieren Sie Ziele.

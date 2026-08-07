#pragma once

#include <iostream>
#include <macroparser/macroparser.hpp>
#include <string>

inline void test()
{
    std::cout << "Running Debug Mode\n";

    // RUN Tests

    std::string test_samfile_content = R"(
#define VERSION 2

#ifdef SECTION_SAMFILE

# Build and run macro parser
mp:
    CD libs/macroparser
    RUN cmake -S . -B build
    RUN ln -sf build/compile_commands.json compile_commands.json
    RUN cmake --build build
    RUN ./build/mylib_test

# Build the engine with CMake
build:
    RUN cmake -S . -B build
    RUN ln -sf build/compile_commands.json compile_commands.json
    RUN cmake --build build

b:
    TASK build

be:
    RUN cmake --build build
    RUN ./build/mygame

# Task to compile the Go DLL (not used yet)
buildgo:
    CD libs/libgodll
    ENV CC="zig cc"
    ENV CXX="zig c++"
    SHELL go build -buildmode=c-shared -o libgo.dll main.go

run:
    TASK build
    RUN ./build/mygame

r:
    TASK run

buildcrablang:
    CD libs/libcrablang
    RUN cargo build --release
    CD ../..
    CP libs/libcrablang/target/release/libcrablang.dll out/build/x64-Debug/libcrablang.dll
    CP libs/libcrablang/target/release/libcrablang.dll out/build/x64-Release/libcrablang.dll
    CP libs/libcrablang/target/release/libcrablang.dll export/libcrablang.dll

# Task for the export
export:
    TASKWIN exportwin
    TASKLIN exportlin

exportlin:
    ECHO "Running Export for Linux"
    RM export
    MKDIR export
    #TASK buildcrablang
    TASK helpsite
    TASK buildsinglepages
    CP resources export/resources
    CP build/mygame export/mygame
    WRITE export/mygame_climenu "#!/bin/bash\n\necho \"Running in Cli Selection Mode:\"\n\nmygame --no-ui --select-menu\n\npause\n"
    WRITE export/mygame_cli "#!/bin/bash\n\necho \"Running in Cli Mode:\"\n\nmygame --no-ui \"$@\"\n"
    WRITE export/fling "#!/bin/bash\n\necho \"Running FLING SOURCE:\"\necho.\n\nmygame --no-ui fling \"$@\"\\n"
    WRITE export/lua "#!/bin/bash\n\necho \"Running LUA SOURCE:\"\necho.\n\nmygame --no-ui lua \"$@\"\\n"
    WRITE export/lua "#!/bin/bash\n\nmygame --no-ui subm \"$@\"\\n"

# Task for the Windows Export
exportwin:
    ECHO "Running Export for Windows"
    RM export
    MKDIR export
    #TASK buildcrablang
    TASK helpsite
    TASK buildsinglepages
    CP resources export/resources
    CP out/build/x64-Release/mygame.exe export/mygame.exe
    WRITE export/open_a_console_here.bat "@cmd\n"
    WRITE export/mygame_climenu.bat "@echo off\n\necho Running in Cli Selection Mode:\n\ncall mygame --no-ui --select-menu\n\npause\n"
    WRITE export/mygame_cli.bat "@echo off\n\necho Running in Cli Mode:\n\ncall mygame --no-ui %*\n"
    WRITE export/fling.bat "@echo off\n\necho Running FLING SOURCE:\necho.\n\ncall mygame --no-ui fling %*\n"
    WRITE export/lua.bat "@echo off\n\necho Running LUA SOURCE:\necho.\n\ncall mygame --no-ui lua %*\n"
    WRITE export/git-subm.bat "@echo off\n\ncall mygame --no-ui subm %*\n"

e:
    TASK export

# Init Task
init:
    RUN l2 clonesubm
    RM resources/editor/htmlpages/.git
    RM resources/editor/htmlpages/.gitattributes
    RUN bun i

i:
    TASK init

# to build to static HTML Page
helpsite:
    RUN l2 view
    MV .samengine/links.md libs/docs/pages/links.md
    CD libs/docs
    RUN bun sam-cli minisite
    MV index.html ../../resources/editor/notice.html
    CD ../../

# Build Single Pages
buildsinglepages:
	ECHO RUN THIS COMMAND ONLY BEFORE RELEASES !!!
    CD libs
    CD vue
    RUN bun i
    RUN bun run build
    CD ..
    CD react
    RUN bun i
    RUN bun run build
    CD ..
    CD svelte
    RUN bun i
    RUN bun run build
    CD ..
    CD ..
    MV libs/vue/dist/index.html resources/editor/htmlpages/vue.html
    MV libs/react/dist/index.html resources/editor/htmlpages/react.html
    MV libs/svelte/dist/index.html resources/editor/htmlpages/svelte.html

bs:
    TASK buildsinglepages

bl:
    TASK buildcrablang

// Done
#endif


#ifdef SECTION_FLING

// This is fling Code
print("Running FLING SOURCE:")

#endif


#ifdef SECTION_BATCH2
echo Hallo
echo DOne
#endif


#ifdef SECTION_DATA

#endif


#ifdef SECTION_SETTINGS
# Done
#endif


)";

    MacroParser parser2;
    parser2.add_macro("SECTION_FLING", {});

    std::cout << parser2.parse_macros(test_samfile_content) << std::endl;

    parser2.remove_macro("SECTION_FLING");
    parser2.add_macro("SECTION_SAMFILE", {});

    std::cout << parser2.parse_macros(test_samfile_content) << std::endl;
}

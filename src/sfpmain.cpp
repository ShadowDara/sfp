// Entry Point for the SAMFILE Parser

#include "settings.hpp"
#include "version.hpp"
#include <batch2.hpp>
#include <cassert>
#include <iostream>
#include <kvp/kvp2.hpp>
#include <macroparser/macroparser.hpp>
#include <ostream>
#include <sfp/runner.hpp>
#include <strutil.hpp>
#include <unordered_map>

using MAP = std::unordered_map<std::string, std::string>;
using KVPMAP = KeyValueParser2::KeyValueStore<MAP>;

enum class SECTION : std::uint8_t
{
    NONE,
    SAMFILE,
    FLING,
    BATCH2,
    DATA,
    DATA_JSON
};

int help()
{
    std::cout << "Usage: sfp [task]\n"
                 "Run with --help for more infos";
    return 0;
}

int fullhelp()
{
    std::cout << "Help for sfp Version " BUILD_VERSION " at " BUILD_DATE
                 ". Commit: " BUILD_COMMIT "\n";
    return 0;
}

std::string loadsamfile(const std::string &filename)
{
    std::ifstream file(filename);
    if (!file)
    {
        std::cerr << RED << "Error while reading samfile" << END << "\n";
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int run_samfile2(const std::string &content, const Config &conf,
                 const std::string &cmd)
{
    RuntimeState state{.path = std::filesystem::current_path().string(),
                       .env_vars = {}};

    Tasks tasks = parse(content, conf);

    // Check for cycled dependencies
    validate_all(tasks);

    // already visited tasks
    std::unordered_set<std::string> visited;

    // Execute task
    return run_task(tasks, cmd, visited, state, conf);
}

int main(int argc, char *argv[])
{
#pragma region DEBUG TESTS

#ifndef NNDEBUG
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
#endif

#pragma endregion

    std::string arg1 = (argc > 1) ? argv[1] : "";

    // Check for bat2 to execute them
    // When the first arg ends with .bat2
    if (strutil::ends_with(arg1, ".bat2"))
    {
        std::ifstream file(arg1);
        if (!file)
        {
            std::cerr << RED << "Error while reading bat2 file" << END << "\n";
            return 1;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();
        auto tokens = batch2::tokenize(content);
        batch2::Interpreter2 interp;
        return interp.execute(tokens);
    }

    // Check for macroparsing
    // pm -> parse macros
    if (arg1 == "-pm")
    {
        if (argc >= 4)
        {
            std::ifstream file(argv[2]);
            if (!file)
            {
                std::cerr << RED << "Error while opening file" << END << "\n";
                return 1;
            }
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string con = buffer.str();

            MacroParser parser;
            con = parser.parse_macros(con);

            std::ofstream outfile(argv[3]);
            if (!outfile)
            {
                std::cerr << RED << "Error while opening outfile" << END
                          << "\n";
                return 1;
            }

            outfile << con;

            return 0;
        }

        std::cout << RED << "Missing file name after parse macros" << END
                  << "\n";
        return 1;
    }

    // Print version
    if (arg1 == "--version" || arg1 == "-v")
    {
        std::cout << BUILD_VERSION "\n";
        return 0;
    }

    // Help
    if (arg1 == "--help" || arg1 == "-h")
    {
        return fullhelp();
    }

    std::string filename;

    if (strutil::ends_with(arg1, ".samfile"))
    {
        filename = arg1;
    }

    MacroParser parser;

    // combine built-in + file
    // std::string content = buildin_samfile_content + "\n\n" + loadsamfile();
    std::string content = loadsamfile(filename);

    // Version of the samfile
    int version = 0;

    // Check if Version exists
    if (parser.contains_macro("VERSION"))
    {
        auto macro = parser.get_macro("VERSION");
        version = std::stoi(macro.body);
    }

    // OLD SAMFILE INTERPRETER
    if (version == 0)
    {
        if (argc < 2)
        {
            std::cerr << RED << "Error: No task specified" << END << "\n";
            return help();
        }

        parser.clear_macros();

        // Run the old interpreter
        return run_samfile2(
            argv[1], {},
            parser.parse_macros(buildin_samfile_content + "\n\n" + content));
    }

    // Samfile Version 2 Interpreter
    if (version == 2)
    {
        if (argc < 2)
        {
            parser.add_macro("SECTION_BATCH2", {});
            std::string input = parser.parse_macros(content);
            auto tokens = batch2::tokenize(input);
            batch2::Interpreter2 interp;
            return interp.execute(tokens);
        }

        KVPMAP seting;
        parser.add_macro("SECTION_SETTINGS", {});
        std::string set = parser.parse_macros(content);
        seting = KeyValueParser2::parse_kvp2(set);
        parser.remove_macro("SECTION_SETTINGS");
        parser.add_macro("SECTION_SAMFILE", {});
        return run_samfile2(
            parser.parse_macros(buildin_samfile_content + "\n\n" + content), {},
            argv[1]);
    }

    std::cerr << RED << "Wrong samfile Version selected: " << version << END
              << "\n";
    return 1;
}

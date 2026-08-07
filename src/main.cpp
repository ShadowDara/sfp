// Entry Point for the SAMFILE Parser

#include "flingrunner.hpp"
#include "settings.hpp"
#include "test.hpp"
#include "version.hpp"

#include <sfplib/sections.hpp>
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

// Function to print the small help message when no enough arguments
int help()
{
    std::cout << "Usage: sfp [task]\n"
                 "Run with --help for more infos\n";
    return 0;
}

// Function to print the full help messages
int fullhelp()
{
    std::cout << "Help for sfp Version " BUILD_VERSION " at " BUILD_DATE
                 ". Commit: " BUILD_COMMIT "\n";
    return 0;
}

// Function to load the samfile
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

// Function to run a samfile
int run_samfile2(const std::string &content, const Config &conf,
                 const std::string &cmd)
{
    RuntimeState state{.path = std::filesystem::current_path().string(),
                       .env_vars = {}};

    // std::cout << "Samfile Content\n" << content << "\n";

    Tasks tasks = parse(content, conf);

    // Check for cycled dependencies
    validate_all(tasks);

    // already visited tasks
    std::unordered_set<std::string> visited;

    // Execute task
    return run_task(tasks, cmd, visited, state, conf);
}

int runner(MAP sections, int argc, char *argv[])
{
    // Load the settings
    KVPMAP seting;
    seting = KeyValueParser2::parse_kvp2(sections["SETTINGS"]);
    //O(sections["SETTINGS"])
    //O(sections["FLING"])

    // Check what the settings say before running something
    std::string entry = seting.get("ENTRYPOINT").value_or("BATCH2");
    if (entry == "SAMFILE")
    {
        //P(Samfile)
        auto task = seting.get("ENTRYTASK").value_or("");
        if (task == "")
        {
            std::cerr << "Error\n";
            return 1;
        }

        run_samfile2(sections["SAMFILE"], {}, task);
    }

    if (entry == "FLING")
    {
        //P(FLING)
        runFling(sections["FLING"]);
    }

    if (argc < 2 && entry == "BATCH2")
    {
        //P(BATCH2)
        auto tokens = batch2::tokenize(sections["BATCH2"]);
        batch2::Interpreter2 interp;
        return interp.execute(tokens);
    }

    //O(entry)

    return 1;
}

// Main function to start the program
int main(int argc, char *argv[])
{

#ifdef NNDEBUG
    test();
#endif

    // Check the first argument
    std::string arg1 = (argc > 1) ? argv[1] : "";

    // Check for bat2 to execute them
    // When the first arg ends with .bat2
    //
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

    // Check .fling files to execute them
    // When first arg ends with .fling
    //
    if (strutil::ends_with(arg1, ".fling"))
    {
        std::ifstream file(arg1);
        if (!file)
        {
            std::cerr << RED << "Error while reading fling file" << END << "\n";
            return 1;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        runFling(buffer.str());
    }

    // -
    if (strutil::starts_with(arg1, "-"))
    {
        // Check for macroparsing
        // pm -> parse macros
        //
        if (arg1 == "-pm")
        {
            if (argc >= 4)
            {
                std::ifstream file(argv[2]);
                if (!file)
                {
                    std::cerr << RED << "Error while opening file" << END
                              << "\n";
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
        //
        if (arg1 == "--version" || arg1 == "-v")
        {
            std::cout << BUILD_VERSION "\n";
            return 0;
        }

        // Help
        //
        if (arg1 == "--help" || arg1 == "-h")
        {
            return fullhelp();
        }
    }

    // Name for the loaded samfile
    std::string filename = "samfile";

    // Check if the first arg end with .samfile because then a samfile will
    // be loaded instead of a task
    //
    if (strutil::ends_with(arg1, ".samfile"))
    {
        filename = arg1;
    }

    MacroParser parser;

    // combine built-in + file
    // std::string content = buildin_samfile_content + "\n\n" + loadsamfile();
    std::string content = loadsamfile(filename);

    // parse the content
    content = parser.parse_macros(content);

    // Version of the samfile
    int version = 0;

    // Check if Version exists
    //
    if (parser.contains_macro("VERSION"))
    {
        auto macro = parser.get_macro("VERSION");
        //std::cout << macro.body << "\n";
        version = std::stoi(macro.body);
    }

    //std::cout << "Version: " << version << "\n";

    // Parse Sections
    //
    auto sections = parse_Sections(content);

    // Version 0
    // OLD SAMFILE INTERPRETER
    //
    if (version == 0)
    {
        if (argc < 2)
        {
            std::cerr << RED << "Error: No task specified" << END << "\n";
            return help();
        }

        // Run the old interpreter
        return run_samfile2(buildin_samfile_content + "\n\n" + sections[""], {},
                            argv[1]);
    }

    // Version 2
    // Samfile Version 2 Interpreter
    //
    if (version == 2)
    {
        return runner(sections, argc, argv);
        // return run_samfile2(buildin_samfile_content + "\n\n" +
        //                         sections["SAMFILES"],
        //                     {}, argv[1], seting);
    }

    // An error appeared when the program got until here
    //
    std::cerr << RED << "Wrong samfile Version selected: " << version << END
              << "\n";
    return 1;
}

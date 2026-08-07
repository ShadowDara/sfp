#include "ffi.hpp"

using namespace fling;
using namespace fling::ast;
using namespace fling::lexer;
using namespace fling::parser;
using namespace fling::runtime;
using namespace fling::runtime::envirment;

using namespace std;

// interne Funktion (dein Originalcode)
void runFile(const std::string& filename)
{
    std::ifstream file{ filename };
    if (!file)
    {
        std::cerr << "Could not open file\n";
        return;
    }

    std::string content(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );

    Parser parser;
    auto env = std::make_shared<Environment>(nullptr);

    Program program = parser.produceAST(content);
    auto result = evaluate(program, *env);
}

void runREPL()
{
    std::cout << "REPL for fling:" << std::endl;

    // Variable for the Source Code
    std::string source;

    // Parser for the source
    Parser parser;

    // Define the Envirment for the Language
    auto env = std::make_shared<Environment>(nullptr);
    // envirment::setupStandardEnvironment(*env);

    while (true)
    {
        std::getline(std::cin, source);

        // Produce AST from source Code
        Program program = parser.produceAST(source);

        RuntimeVal result = evaluate(program, *env);
        cout << result.toString() << endl;
    }
}

// RUST WRAPPER FUNCTION
void run_file(rust::Str filename)
{
    std::string file_str(filename);

    runFile(file_str);
}

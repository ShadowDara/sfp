#include "flingrunner.hpp"

using namespace fling;
using namespace fling::ast;
using namespace fling::lexer;
using namespace fling::parser;
using namespace fling::runtime;
using namespace fling::runtime::envirment;

void runREPLImGui()
{
    static char inputBuffer[1024] = "";
    static std::vector<std::string> history;

    static Parser parser;
    static auto env = std::make_shared<Environment>(nullptr);

    std::string source = inputBuffer;

    if (!source.empty())
    {
        try
        {
            // AST bauen
            Program program = parser.produceAST(source);

            // Ausf�hren
            RuntimeVal result = evaluate(program, env);

            // Ausgabe speichern
            history.push_back("> " + source);
            history.push_back(result.toString());
        }
        catch (const std::exception &e)
        {
            history.push_back(std::string("Error: ") + e.what());
        }

        inputBuffer[0] = '\0';
    }
}

// Function to run a File
void runFling(const std::string &content)
{
    Parser parser;
    auto env = std::make_shared<Environment>(nullptr);
    // envirment::setupStandardEnvironment(*env);

    Program program = parser.produceAST(content);
    // std::cout << "Print Program: " << program.toString() << "\n";

    auto result = evaluate(program, env);
    // std::cout << result.toString() << "\n";
}

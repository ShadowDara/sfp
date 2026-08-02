// main.cpp

#include "cpp/frontend/lexer.hpp"
#include "cpp/frontend/parser.hpp"
#include "cpp/frontend/ast.hpp"
#include "cpp/runtime/interpreter.hpp"
#include "cpp/runtime/envirments.hpp"

#include <iostream>
#include <string>
#include <cassert> // Für assert()
#include <fstream>

using namespace fling;
using namespace fling::ast;
using namespace fling::lexer;
using namespace fling::parser;
using namespace fling::runtime;
using namespace fling::runtime::envirment;

using namespace std;


// Hilfsfunktion zum Testen von Tokens
void assertToken(const lexer::Token &token,
    const std::string &expectedValue, lexer::TokenType expectedType)
{
    if (token.type != expectedType || token.value != expectedValue)
    {
        std::cerr << "Token mismatch: got " << token.value 
            << ", expected " << expectedValue << std::endl;
        // edwina war hier und ist nicht geil 
        std::exit(1);
    }
}


// Function to run Tests
void runTests()
{
    {
        std::vector<Token> tokens = tokenize("let y = 100 + 23");
        assert(tokens.size() == 7); // 6 Tokens + Eof
        assertToken(tokens[0], "let", TokenType::Let);
        assertToken(tokens[5], "23", TokenType::Number);
    }

    {
        std::vector<Token> tokens = tokenize("foo = (5 + 3)");
        assert(tokens.size() == 8); // 7 Tokens + Eof
        assertToken(tokens[0], "foo", TokenType::Identifier);
        assertToken(tokens[1], "=", TokenType::Equals);
        assertToken(tokens[2], "(", TokenType::OpenParen);
    }

    std::cout << "Tests 1\n";
}


// Function to run a File
void runFile(const std::string& filename)
{
    std::ifstream file{ filename };
    if (!file)
    {
        std::cerr << "Could not open file" << "\n";
        return;
    }

    std::string content(
        (std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>()
    );

    Parser parser;
    auto env = std::make_shared<Environment>(nullptr);
    // envirment::setupStandardEnvironment(*env);

    Program program = parser.produceAST(content);
	//std::cout << "Print Program: " << program.toString() << "\n";

    auto result = evaluate(program, env);
    //std::cout << result.toString() << "\n";
}


void runREPL()
{
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

        RuntimeVal result = evaluate(program, env);
        cout << result.toString() << endl;
    }
}


// Main function
int main()
{
    std::cout << "Running Fling Tests...\n"
              << std::endl;

    std::vector<Token> tokens = tokenize("let x = 45");

    // Erwartet: 5 Tokens -> 4 + Eof Token at the end of the file
    assert(tokens.size() == 5 && "Expected 5 tokens");

    // Einzelne Tokens überprüfen
    assertToken(tokens[0], "let", TokenType::Let);
    assertToken(tokens[1], "x", TokenType::Identifier);
    assertToken(tokens[2], "=", TokenType::Equals);
    assertToken(tokens[3], "45", TokenType::Number);

    std::cout << "tests passed" << std::endl;

    runTests();

    std::string testthis = "let x = 90 * 90";
    std::cout << testthis << std::endl;
    tokenize(testthis);

    std::cout << "\nAll tests passed!\n"
              << std::endl;

    runFile("../../../testcode.txt");
    runFile("../../../while.test");
    runFile("../../../prime.txt");

	//runREPL();

    return 0;
}

#pragma once

#include "strutil.hpp"
#include <string>
#include <vector>
#include <iostream>
#include <stack>
#include <sstream>
#include <string_view>
#include <unordered_map>

namespace batch2
{

    enum class TokenType
    {
        UNKNOWN = 0,
        Identifier,
        String,

        Newline,

        LeftBrace,
        RightBrace,

        Number,

        Goto,
        Label,
        Echo,
        Pause,

        EndOfFile
    };


    struct Token
    {
        TokenType type;
        std::string value;
    };


    extern std::unordered_map<std::string, TokenType> KEYWORDS;

    std::vector<Token> tokenize(std::string input);


    struct Label
    {
        std::string name;
        size_t tokenNumber;
    };


    class Interpreter2
    {
      private:
        std::stack<std::string> stack;
        std::vector<Label> labels;
        std::vector<Token> tokens;
        size_t currentToken = 0;

        void setupEnvironment();

        void parseLabel();

        std::string resolve(std::string label);

        void parseIf(std::string c);

        void parseTokens();

      public:
        int execute(std::vector<Token> tokens);
    };

} // namespace batch2

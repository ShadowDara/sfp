#include "batch2.hpp"


using namespace batch2;


#pragma region Lexer

/*
 * a list of Keywords and Identifiers for the Language
 */
std::unordered_map<std::string, TokenType> batch2::KEYWORDS = {
    {"goto", TokenType::Goto},
    {"echo", TokenType::Echo},
    {"pause", TokenType::Pause}
};


// peek function
bool match(const std::vector<char> &src, size_t i, char expected)
{
    return i < src.size() && src[i] == expected;
}

bool isAlphaNumeric(char c)
{
    return std::isalnum(static_cast<unsigned char>(c));
}

// Check if string is alphabetic
bool isAlpha(const std::string &src)
{
    for (char c : src)
    {
        if (!std::isalpha(static_cast<unsigned char>(c)))
        {
            return false;
        }
    }
    return true;
}

// Overload: Check if single char is alpha
bool isAlpha(char c) { return std::isalpha(static_cast<unsigned char>(c)); }

// Check if string is integer
bool isInt(const std::string &str)
{
    if (str.empty())
        return false;
    for (char c : str)
    {
        if (!std::isdigit(static_cast<unsigned char>(c)))
        {
            return false;
        }
    }
    return true;
}


// Function to check if a character is an integer
bool isInt(char c) { return std::isdigit(static_cast<unsigned char>(c)); }


// Check for Newline
static bool isNewline(char c) { return c == '\n' || c == '\r'; }


// Whitespace check
static bool isSkippable(char c) { return c == ' ' || c == '\t'; }


Token token(TokenType type, std::string value)
{
    return Token{type, value};
}


std::vector<Token> batch2::tokenize(std::string input)
{
    std::vector<Token> tokens;
    std::vector<char> src(input.begin(), input.end());
    
    for (size_t i = 0; i < src.size();)
    {
        char current = src[i];

        if (isNewline(current))
        {
            tokens.push_back(token(TokenType::Newline, "\n"));
            i++;
            continue;
        }

        if (isSkippable(current))
        {
            i++;
            continue;
        }

        switch (current)
        {
        case ':':
            tokens.push_back(token(TokenType::Label, ":"));
            i++;
            break;
        case '(':
            tokens.push_back(token(TokenType::LeftBrace, "("));
            i++;
            break;

        case ')':
            tokens.push_back(token(TokenType::RightBrace, ")"));
            i++;
            break;

        default:
            // Zahl aufbauen
            if (isInt(current))
            {
                std::string num;
                while (i < src.size() && isInt(src[i]))
                {
                    num += src[i];
                    i++;
                }
                tokens.push_back(token(TokenType::Number, num));
            }

            // Identifier aufbauen
            else if (isAlpha(current))
            {
                std::string ident;

                // erstes Zeichen MUSS Buchstabe sein
                ident += current;
                i++;

                // danach Buchstaben ODER Zahlen
                while (i < src.size() && isAlphaNumeric(src[i]))
                {
                    ident += src[i];
                    i++;
                }

                // Keyword check bleibt gleich
                if (KEYWORDS.find(ident) != KEYWORDS.end())
                {
                    tokens.push_back(token(KEYWORDS[ident], ident));
                }
                else
                {
                    tokens.push_back(token(TokenType::Identifier, ident));
                }
            }

            else
            {
                std::cout << "Unrecognized character in source: " << current
                          << " ASCII: " << (int)current << std::endl;

                // Return empty Vector
                return {};
            }

            break;
        }
    }

    return tokens;
}

#pragma endregion

#pragma region Interpreter2

void batch2::Interpreter2::setupEnvironment()
{
    // for the End Label
    labels.push_back({"end", static_cast<size_t>(-1)});
}

void batch2::Interpreter2::parseLabel()
{
    for (size_t i = 0; i < tokens.size(); i++)
    {
        if (tokens[i].type == TokenType::Label && (i + 1) < tokens.size())
        {
            i++;
            labels.push_back({tokens[i].value, i + 1});
        }
    }
}

std::string batch2::Interpreter2::resolve(std::string label) { return label; }

void batch2::Interpreter2::parseIf(std::string c)
{
    auto pos = c.find("==");

    if (pos == std::string::npos)
        throw std::runtime_error("missing ==");

    auto trim = [](std::string str)
    {
        size_t b = str.find_first_not_of(" \t");
        size_t e = str.find_last_not_of(" \t");

        if (b == std::string::npos)
            return std::string();

        return str.substr(b, e - b + 1);
    };

    std::string left = resolve(trim(c.substr(0, pos)));
    std::string right = resolve(trim(c.substr(pos + 2)));
}

void batch2::Interpreter2::parseTokens()
{
    switch (tokens[currentToken].type)
    {
    case TokenType::Label:
        currentToken++; // Skip the label token
        if (currentToken < tokens.size() &&
            tokens[currentToken].type == TokenType::Identifier)
        {
            currentToken++; // Skip the identifier token
        }
        break;
    case TokenType::Echo:
    {
        currentToken++;

        std::string text;

        while (currentToken < tokens.size() &&
               tokens[currentToken].type != TokenType::Newline)
        {
            if (!text.empty())
                text += " ";

            text += tokens[currentToken].value;

            currentToken++;
        }

        std::cout << resolve(text) << "\n";

        break;
    }
    case TokenType::Pause:
        currentToken++;
        std::cout << "Press Enter to continue...";
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        break;
    case TokenType::Goto:
        currentToken++;
        if (currentToken < tokens.size() &&
            tokens[currentToken].type == TokenType::Identifier)
        {
            std::string label = resolve(tokens[currentToken].value);
            auto it =
                std::find_if(labels.begin(), labels.end(),
                             [&](const Label &l) { return l.name == label; });
            if (it != labels.end())
            {
                currentToken = it->tokenNumber;
            }
            else
            {
                throw std::runtime_error("Label not found: " + label);
            }
        }
        break;
    default:
        currentToken++;
        break;
    }
    }

int batch2::Interpreter2::execute(std::vector<Token> t)
    {
    this->tokens = t;

    this->parseLabel();

    while (currentToken < tokens.size())
    {
        parseTokens();
    }

    return 0;
}

#pragma endregion

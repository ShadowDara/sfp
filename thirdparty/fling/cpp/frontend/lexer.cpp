// lexer.cpp

#include "lexer.hpp"

namespace fling
{
    namespace lexer
    {
        // Debug Printing
        std::string tokenTypeToString(TokenType type)
        {
            switch (type)
            {
            case TokenType::Number:
                return "Number";
            case TokenType::Identifier:
                return "Identifier";
            case TokenType::Equals:
                return "Equals";
            case TokenType::OpenParen:
                return "OpenParen";
            case TokenType::CloseParen:
                return "CloseParen";
            case TokenType::BinaryOperator:
                return "BinaryOperator";
            case TokenType::Let:
                return "Let";
            case TokenType::If:
                return "If";
            case TokenType::Else:
                return "Else";
            case TokenType::While:
                return "While";
            default:
                return "Unknown";
            }
        }

        std::ostream &operator<<(std::ostream &os, const Token &token)
        {
            os << "Token(type=\"" << tokenTypeToString(token.type)
                << "\", value=\"" << token.value << "\") Line: " << token.line
                << " Column: " << token.column;
            return os;
        }

        // peek function
        bool match(const std::vector<char>& src, size_t i, char expected)
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
        bool isAlpha(char c)
        {
            return std::isalpha(static_cast<unsigned char>(c));
        }

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
        bool isInt(char c)
        {
            return std::isdigit(static_cast<unsigned char>(c));
        }

        // Check for Newline
        bool isNewline(char c)
        {
            return c == '\n' || c == '\r';
		}

        // Whitespace check
        bool isSkippable(char c)
        {
            return c == ' ' || c == '\t';
        }

        // Token-Erzeuger
        Token token(const std::string &value, TokenType type, int line, int column)
        {
            return Token{value, type, line, column};
        }

        /*
         * a list of Keywords and Identifiers for the Language
         */
        std::unordered_map<std::string, TokenType> KEYWORDS = {
            {
                "let",
                TokenType::Let
            },
            {
                "const",
                TokenType::Const
            },
            {
                "fn",
                TokenType::Fn
            },
            {
                "if",
                TokenType::If
            },
            {
                "else",
                TokenType::Else
            },
            {
                "while",
                TokenType::While
            }
        };

        // Tokenizer
        std::vector<Token> tokenize(const std::string &sourceCode)
        {
            std::vector<Token> tokens;
            std::vector<char> src(sourceCode.begin(), sourceCode.end());

            int line = 1;
            int column = 1;

            for (size_t i = 0; i < src.size();)
            {
                char current = src[i];

                column++;

                if (isNewline(current))
                {
                    column = 1;
                    line++;
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

                // Opening Parenthesis
                case '(':
                    tokens.push_back(token("(", TokenType::OpenParen, line, column));
                    i++;
                    break;

                // Closing Parenthesis
                case ')':
                    tokens.push_back(token(")", TokenType::CloseParen, line, column));
                    i++;
                    break;

				// Opening Curly Brace
                case '{':
                    tokens.push_back(token("{", TokenType::OpenCurlyBrace, line, column));
                    i++;
					break;

				// Closing Curly Brace
                case '}':
                    tokens.push_back(token("}", TokenType::CloseCurlyBrace, line, column));
					i++;
                    break;

                // Opening Squared Brace
                case '[':
                    tokens.push_back(token("[", TokenType::OpenSquaredBrace, line, column));
                    i++;
                    break;

                    // Closing Squared Brace
                case ']':
                    tokens.push_back(token("]", TokenType::CloseSquaredBrace, line, column));
                    i++;
                    break;

                // Addition Operator
                case '+':
                    tokens.push_back(token(std::string(1, current),
                        TokenType::BinaryOperator, line, column));
                    i++;
                    break;

                // Subtraction Operator
                case '-':
                    tokens.push_back(token(std::string(1, current),
                        TokenType::BinaryOperator, line, column));
                    i++;
                    break;

                // Multiplication Operator
                case '*':
                    tokens.push_back(token(std::string(1, current),
                        TokenType::BinaryOperator, line, column));
                    i++;
                    break;
                
                // # for Comments
                case '#':
                {
                    // Kommentar erkennen
                    
                    // Skip "#"
                    i++;

                    // Alles bis Zeilenende überspringen
                    while (i < src.size() && !isNewline(src[i]))
                    {
                        i++;
                    }

                    break; // nichts tokenizen!
                }

                // Division Operator
                case '/':
                {
                    // Kommentar erkennen
                    if (i + 1 < src.size() && src[i + 1] == '/')
                    {
                        // Skip "//"
                        i += 2;

                        // Alles bis Zeilenende überspringen
                        while (i < src.size() && !isNewline(src[i]))
                        {
                            i++;
                        }

                        break; // nichts tokenizen!
                    }

                    // Normaler Division-Operator
                    tokens.push_back(token("/", TokenType::BinaryOperator, line, column));
                    i++;
                    break;
                }

                // Modulo Operator
                case '%':
                    tokens.push_back(token(std::string(1, current),
                        TokenType::BinaryOperator, line, column));
                    i++;
                    break;
                
                // Smaller
                case '<':
                {
                    if (i + 1 < src.size() && src[i + 1] == '=')
                    {
                        tokens.push_back(token("<=", TokenType::BinaryOperator, line, column));
                        i += 2;
                    }
                    else
                    {
                        tokens.push_back(token("<", TokenType::BinaryOperator, line, column));
                        i++;
                    }
                    break;
                }

                // Bigger
                case '>':
                {
                    if (i + 1 < src.size() && src[i + 1] == '=')
                    {
                        tokens.push_back(token(">=", TokenType::BinaryOperator, line, column));
                        i += 2;
                    }
                    else
                    {
                        tokens.push_back(token(">", TokenType::BinaryOperator, line, column));
                        i++;
                    }
                    break;
                }

                // Assignment Operator
                case '=':
                {
                    if (i + 1 < src.size() && src[i + 1] == '=')
                    {
                        tokens.push_back(token("==", TokenType::BinaryOperator, line, column));
                        i += 2;
                    }
                    else
                    {
                        tokens.push_back(token("=", TokenType::Equals, line, column));
                        i++;
                    }
                    break;
                }

                case '!':
                {
                    if (i + 1 < src.size() && src[i + 1] == '=')
                    {
                        tokens.push_back(token("!=", TokenType::BinaryOperator, line, column));
                        i += 2;
                    }
                    else
                    {
                        tokens.push_back(token("!", TokenType::BinaryOperator, line, column));
                        i++;
                    }
                    break;
                }

                case '&':
                {
                    if (i + 1 < src.size() && src[i + 1] == '&')
                    {
                        tokens.push_back(token("&&", TokenType::BinaryOperator, line, column));
                        i += 2;
                    }
                    else
                    {
                        std::cerr << "Unexpected '&'\n";
                        i++;
                    }
                    break;
                }

                case '|':
                {
                    if (i + 1 < src.size() && src[i + 1] == '|')
                    {
                        tokens.push_back(token("||", TokenType::BinaryOperator, line, column));
                        i += 2;
                    }
                    else
                    {
                        std::cerr << "Unexpected '|'\n";
                        i++;
                    }
                    break;
                }

                // Comma
                case ',':
                    tokens.push_back(token(",", TokenType::Comma, line, column));
                    i++;
                    break;

                // Dot
                case '.':
                    tokens.push_back(token(".", TokenType::Dot, line, column));
                    i++;
                    break;

                // Colon
                case ':':
                    tokens.push_back(token(":", TokenType::Colon, line, column));
                    i++;
                    break;
                
                // Semicolon
                case ';':
                    tokens.push_back(token(";", TokenType::Semicolon, line, column));
                    i++;
                    break;

                // String Literal
                case '"':
                {
                    std::string str = "";
                    i++; // Skip opening quote
                    while (i < src.size() && src[i] != '"')
                    {
                        str += src[i];
                        i++;
                        column++;
                    }
                    if (i < src.size())
                    {
                        i++; // Skip closing quote
                        column++;
                    }
                    tokens.push_back(token(str, TokenType::String, line, column));
                    break;
                }

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
                        tokens.push_back(token(num, TokenType::Number, line, column));
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
                            tokens.push_back(token(ident, KEYWORDS[ident], line, column));
                        }
                        else
                        {
                            tokens.push_back(token(ident, TokenType::Identifier, line, column));
                        }
                    }

                    else
                    {
                        std::cout << "Unrecognized character in source: "
                            << current << " ASCII: " << (int)current << std::endl;
                        
                        // Return empty Vector
                        return {};
                    }

                    break;
                }
            }

            tokens.push_back(token("EndOfFile", TokenType::Eof, line, column));

            // // Debug Output
            // std::cout << "Debug Tokens:" << std::endl;
            // for (const auto &tok : tokens)
            // {
            //     std::cout << tok << std::endl;
            // }

            return tokens;
        }

    } // namespace lexer
} // namespace fling

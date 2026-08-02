// parser.cpp

#include "parser.hpp"

using namespace std;
using namespace fling;

namespace fling
{
    namespace parser
    {
        // match the namespace in the header

        // Implementation of Parser member functions

        // Function to check if end of file is reached
        bool Parser::not_eof()
        {
            return !tokens.empty() &&
                   tokens[0].type != lexer::TokenType::Eof;
        }

        // Function to get the current token
        fling::lexer::Token Parser::at()
        {
            return this->tokens[0];
        }

        // Function to get the current token and remove it from the List
        fling::lexer::Token Parser::eat()
        {
            // save current
            fling::lexer::Token prev = this->tokens[0];

            // remove first element
            this->tokens.erase(this->tokens.begin());

            return prev;
        }

        // Function to expect a specific Token Type
        fling::lexer::Token Parser::expect(fling::lexer::TokenType type, const std::string &context)
        {
            lexer::Token tk = at();

            if (tk.type != type)
            {
                std::cerr << "\n=== PARSER ERROR ===\n";

                std::cerr << "Context: " << context << "\n";

                std::cerr << "Expected: " << tokenTypeToString(type)
                          << " ASCII: "
                          << (tokenTypeToString(type).empty() ? 0 : int(tokenTypeToString(type)[0]))
                          << "\n";

                std::cerr << "Found: " << tokenTypeToString(tk.type)
                          << " (\"" << tk.value << "\") ASCII: ";
                for (char c : tk.value)
                    std::cerr << int(c) << " ";
                std::cerr << "\n";

                std::cerr << "Location: line " << tk.line
                          << ", column " << tk.column << "\n";

                std::cerr << "====================\n";

                std::exit(1);
            }

            return eat();
        }

        // Funktion to parse a Statement
        std::unique_ptr<fling::ast::Stmt> Parser::parse_stmt()
        {
            switch (this->at().type)
            {
            // Let
            case (lexer::TokenType::Let):
                return parse_var_declaration();

            // Const
            case (lexer::TokenType::Const):
                return parse_var_declaration();
            
            // If
            case (lexer::TokenType::If):
                return parse_if_statement();
            
            // While
            case (lexer::TokenType::While):
                return parse_while_statement();

            // Function
            case (lexer::TokenType::Fn):
                return parse_fn_declaration();

            // Default
            default:
                // For now, we only parse expressions (identifiers / literals)
                // and wrap them as statements.
                return parse_expr();
            }
        }

        // Parse a block of statements inside {...}
        std::unique_ptr<fling::ast::Stmt> Parser::parse_stmt_block()
        {
            auto block = std::make_unique<fling::ast::Program>();
            while (this->at().type != lexer::TokenType::CloseCurlyBrace && this->at().type != lexer::TokenType::Eof)
            {
                block->body.push_back(this->parse_stmt());
            }
            this->expect(lexer::TokenType::CloseCurlyBrace, "Block muss mit '}' geschlossen werden");
            return block;
        }

        // Parse if statement: if <expr> { ... } [else { ... }]
        std::unique_ptr<fling::ast::Stmt> Parser::parse_if_statement()
        {
            this->eat(); // eat 'if'
            // Parse condition (alles bis zur nächsten öffnenden Klammer)
            auto condition = this->parse_expr();
            this->expect(lexer::TokenType::OpenCurlyBrace, "Erwarte '{' nach if-Bedingung");
            auto thenBranch = this->parse_stmt_block();

            std::unique_ptr<fling::ast::Stmt> elseBranch = nullptr;
            if (this->at().type == lexer::TokenType::Else) {
                this->eat(); // eat 'else'
                this->expect(lexer::TokenType::OpenCurlyBrace, "Erwarte '{' nach else");
                elseBranch = this->parse_stmt_block();
            }

            auto node = std::make_unique<fling::ast::IfStatement>();
            node->condition = std::move(condition);
            node->thenBranch = std::move(thenBranch);
            node->elseBranch = std::move(elseBranch);
            return node;
        }

        // Parse While Statement
        std::unique_ptr<fling::ast::Stmt> Parser::parse_while_statement()
        {
            this->eat(); // eat 'while'

            // condition
            auto condition = this->parse_expr();

            // body
            this->expect(
                lexer::TokenType::OpenCurlyBrace,
                "Expected '{' after while condition"
            );

            auto body = this->parse_stmt_block();

            auto node = std::make_unique<fling::ast::WhileStatement>();
            node->condition = std::move(condition);
            node->body = std::move(body);

            return node;
        }

        // Function to parse a Function Declaration
        std::unique_ptr<fling::ast::Stmt> Parser::parse_fn_declaration()
        {
            this->eat(); // eat the fn keyword
            auto name = this->expect(lexer::TokenType::Identifier,
                                     "Expected Function name following fn keyword")
                            .value;

            auto args = this->parse_agrs();
            std::vector<std::string> params;
            for (const auto &arg : args)
            {
                if (arg->kind != ast::NodeType::Identifier)
                {
                    std::cout << arg->toString() << "\n";
                    std::cerr
                        << "Inside function declaration expected parameters to be of type string."
                        << std::endl;
                    return nullptr;
                }

                auto *ident = static_cast<ast::Identifier *>(arg.get());
                params.push_back(ident->symbol);
            }

            this->expect(lexer::TokenType::OpenCurlyBrace,
                         "Expected function body following declaration");

            std::vector<std::unique_ptr<ast::Stmt>> body = std::vector<std::unique_ptr<ast::Stmt>>();
            while (this->at().type != lexer::TokenType::Eof &&
                   this->at().type != lexer::TokenType::CloseCurlyBrace)
            {
                body.push_back(std::move(this->parse_stmt()));
            }

            // End of function Body
            this->expect(lexer::TokenType::CloseCurlyBrace,
                         "Cloaing Brace expected inside function declaration");

            auto fn = ast::FunctionDeclaration(name, std::move(body), params);
            std::unique_ptr<ast::FunctionDeclaration> fnr =
                std::make_unique<ast::FunctionDeclaration>(std::move(fn));

            return fnr;
        }

        // Function to declare a new Variable
        //
        // (CONST | LET) IDENTIFIER ;
        //
        // (CONST | LET) IDENTIFIER = EXPR ;
        //
        std::unique_ptr<fling::ast::Stmt> Parser::parse_var_declaration()
        {
            bool isConstant =
                (this->eat().type == lexer::TokenType::Const);

            std::string identifier = this->expect(lexer::TokenType::Identifier,
                                                  "Expected identifier name following let | const keywords")
                                         .value;

            if (this->at().type == lexer::TokenType::Semicolon)
            {
                this->eat(); // Expect the Semicolon
                if (isConstant)
                {
                    std::cout << "Must assigne value to a Constant expression. No value provided."
                              << std::endl;

                    // Return Nullpointer
                    return nullptr;
                }

                auto var = std::make_unique<ast::VarDeclaration>();

                var->constant = isConstant;
                var->identifier = identifier;
                var->value = nullptr;

                return var;
            }

            this->expect(lexer::TokenType::Equals,
                         "Expected equals Token following identifier in var declaration.");

            auto declaration = std::make_unique<ast::VarDeclaration>();

            declaration->value = this->parse_expr();
            declaration->constant = isConstant;
            declaration->identifier = identifier;

            this->expect(lexer::TokenType::Semicolon,
                         "Variable declaration statement must end with semicolon.");

            return declaration;
        }

        // Funktion to parse an expression
        std::unique_ptr<fling::ast::Expr> Parser::parse_expr()
        {
            return this->parse_assignment_expr();
        }

        // &&, ||
        std::unique_ptr<fling::ast::Expr> Parser::parse_logical_expr()
        {
            auto left = this->parse_comparison_expr();

            while (
                this->at().value == "&&" ||
                this->at().value == "||"
            )
            {
                std::string op = this->eat().value;
                auto right = this->parse_comparison_expr();

                auto expr = std::make_unique<fling::ast::BinaryExpr>();
                expr->left = std::move(left);
                expr->right = std::move(right);
                expr->callculation_operator = op;

                left = std::move(expr);
            }

            return left;
        }

        // <, > == etc
        std::unique_ptr<fling::ast::Expr> Parser::parse_comparison_expr()
        {
            auto left = this->parse_additive_expr();

            while (
                this->at().value == "==" ||
                this->at().value == "!=" ||
                this->at().value == "<"  ||
                this->at().value == ">"  ||
                this->at().value == "<=" ||
                this->at().value == ">="
            )
            {
                std::string op = this->eat().value;
                auto right = this->parse_additive_expr();

                auto expr = std::make_unique<fling::ast::BinaryExpr>();
                expr->left = std::move(left);
                expr->right = std::move(right);
                expr->callculation_operator = op;

                left = std::move(expr);
            }

            return left;
        }

        // Function to parse an assignment Expression
        std::unique_ptr<fling::ast::Expr> Parser::parse_assignment_expr()
        {
            auto left = this->parse_logical_expr();
            // switch this out with object Expression

            if (this->at().type == lexer::TokenType::Equals)
            {
                this->eat(); // eat the equals token
                auto right = this->parse_assignment_expr();
                auto assignExpr = std::make_unique<ast::AssignmentExpr>();
                assignExpr->assignme = std::move(left); // copy left expression
                assignExpr->value = std::move(right);
                return assignExpr;
            }

            return left;
        }

        // Function to parse an Object Expression
        std::unique_ptr<fling::ast::Expr> Parser::parse_object_expr()
        {
            // Wird NUR aufgerufen wenn wir sicher '{' haben
            this->expect(lexer::TokenType::OpenCurlyBrace, "Expected '{' to start object literal");

            std::vector<std::unique_ptr<ast::Property>> properties;

            while (this->not_eof() && this->at().type != lexer::TokenType::CloseCurlyBrace)
            {
                // { key }

                // Wenn das nächste Token ein Komma ist, überspringen wir es (optional trailing comma)
                if (this->at().type == lexer::TokenType::Comma)
                {
                    this->eat();
                    // Wenn nach dem Komma direkt CloseCurlyBrace kommt -> Ende
                    if (this->at().type == lexer::TokenType::CloseCurlyBrace)
                    {
                        break;
                    }
                }

                std::string key = this->expect(lexer::TokenType::Identifier,
                                               "Expected identifier as key in object literal")
                                      .value;

                if (this->at().type == lexer::TokenType::Colon)
                {
                    this->eat(); // eat the colon
                    auto value = this->parse_expr();
                    auto property = std::make_unique<ast::Property>();
                    property->key = key;
                    property->value = std::move(value);
                    properties.push_back(std::move(property));
                    continue;
                }
                else if (this->at().type == lexer::TokenType::Comma)
                {
                    this->eat(); // eat the comma
                    // Shorthand Property: { key }
                    auto property = std::make_unique<ast::Property>();
                    property->key = key;
                    property->value = std::make_unique<ast::Identifier>(key); // value is the identifier itself
                    properties.push_back(std::move(property));
                    continue;
                }

                // { key: value }
                this->expect(lexer::TokenType::Colon,
                             "Missing colon ':' after key in object literal");
                auto value = this->parse_expr();

                auto property = std::make_unique<ast::Property>();
                property->key = key;
                property->value = std::move(value);
                properties.push_back(std::move(property));

                if (this->at().type != lexer::TokenType::CloseCurlyBrace)
                {
                    this->expect(lexer::TokenType::Comma,
                                 "Expected Comma or Closing Bracket following property");
                }
            }

            this->expect(lexer::TokenType::CloseCurlyBrace,
                         "Object Literal Closing Brace { missing!");

            auto objLiteral = std::make_unique<ast::ObjectLiteral>();
            objLiteral->properties = std::move(properties);

            return objLiteral;
        }

        // Function to parse an additive Expression
        std::unique_ptr<fling::ast::Expr> Parser::parse_additive_expr()
        {
            auto left = parse_multiplicitave_expr();

            // Null Check
            assert(left != nullptr);

            // For Addition and Subtraction
            while (this->at().value == "+" || this->at().value == "-")
            {
                std::string callculation_operator = this->eat().value;
                auto right = parse_multiplicitave_expr();

                // Null Check
                assert(right != nullptr);

                auto leftnew = std::make_unique<fling::ast::BinaryExpr>();
                leftnew->left = std::move(left);
                leftnew->right = std::move(right);
                leftnew->callculation_operator = callculation_operator;

                left = std::move(leftnew);
            }

            return left;
        }

        // Function to parse an multiplicitave Expression
        std::unique_ptr<fling::ast::Expr> Parser::parse_multiplicitave_expr()
        {
            auto left = parse_unary_expr();

            // Null Check
            assert(left != nullptr);

            // For Division, Multiplication and Modulo
            while (
                this->at().value == "/" ||
                this->at().value == "*" ||
                this->at().value == "%")
            {
                std::string callculation_operator = this->eat().value;
                auto right = this->parse_unary_expr();

                // Null Check
                assert(right != nullptr);

                auto leftnew = std::make_unique<fling::ast::BinaryExpr>();
                leftnew->left = std::move(left);
                leftnew->right = std::move(right);
                leftnew->callculation_operator = callculation_operator;

                left = std::move(leftnew);

                // Null Check
                assert(left != nullptr);
            }

            return left;
        }

        // Unary Expression !true, -x
        std::unique_ptr<fling::ast::Expr> Parser::parse_unary_expr()
        {
            if (
                this->at().value == "!" ||
                this->at().value == "-"
            )
            {
                std::string op = this->eat().value;

                auto expr = std::make_unique<ast::UnaryExpr>();

                // Null Check
                assert(expr != nullptr);

                expr->op = op;
                expr->operand = this->parse_unary_expr(); // RECURSIVE (wichtig!)

                return expr;
            }

            auto r = this->parse_call_member_expr();

            // Null Check
            assert(r != nullptr);

            return r;
        }

        // Function to parse a Call Member Expression
        std::unique_ptr<fling::ast::Expr> Parser::parse_call_member_expr()
        {
            auto member = parse_member_expr();

            // Null Check
            assert(member != nullptr);

            if (this->at().type == lexer::TokenType::OpenParen)
            {
                return parse_call_expr(std::move(member));
            }

            return member;
        }

        // Function to parse a Call Expression
        std::unique_ptr<fling::ast::Expr> Parser::parse_call_expr(
            std::unique_ptr<fling::ast::Expr> caller)
        {
            auto callExpr = std::make_unique<ast::CallExpr>(std::move(caller), parse_agrs());

            if (at().type == lexer::TokenType::OpenParen)
            {
                auto callExpr2 = std::move(parse_call_expr(std::move(callExpr)));
                return callExpr2;
            }

            return callExpr;
        }

        // Function to parse arguments for a Call Expression
        std::vector<std::unique_ptr<fling::ast::Expr>> Parser::parse_agrs()
        {
            // This call is not required to tbh
            expect(lexer::TokenType::OpenParen,
                   "Expected opening parenthesis for function call arguments");

            auto agrs = std::vector<std::unique_ptr<ast::Expr>>();

            if (this->at().type != lexer::TokenType::CloseParen)
            {
                agrs = parse_argument_list();
            }

            expect(lexer::TokenType::CloseParen,
                   "Expected closing parenthesis after function call arguments");

            return agrs;
        }

        // Function to parse the Argument List
        std::vector<std::unique_ptr<fling::ast::Expr>> Parser::parse_argument_list()
        {
            std::vector<std::unique_ptr<ast::Expr>> agrs;
            agrs.push_back(std::move(parse_assignment_expr()));

            while (this->at().type == lexer::TokenType::Comma)
            {
                this->eat(); // eat the comma
                auto expr = parse_assignment_expr();
                agrs.push_back(std::move(expr));
            }

            return agrs;
        }

        // Function to parse a Member Expression
        std::unique_ptr<fling::ast::Expr> Parser::parse_member_expr()
        {
            auto object = parse_primary_expr();

            // Null Check
            assert(object != nullptr);

            while (at().type == lexer::TokenType::Dot || at().type == lexer::TokenType::OpenSquaredBrace)
            {
                auto theoperator = eat();
                std::unique_ptr<ast::Expr> property;
                bool computed = false;

                if (theoperator.type == lexer::TokenType::Dot)
                {
                    computed = false;
                    // get identifier after dot
                    property = std::move(parse_primary_expr());

                    if ((*property).kind != ast::NodeType::Identifier)
                    {
                        std::cerr
                            << "Can not use dot operator without right hand side beeing a identifier"
                            << std::endl;
                        std::exit(1);
                    }
                }
                else
                {
                    // this allows obs[computedValue]
                    computed = true;
                    property = std::move(parse_expr());
                    expect(lexer::TokenType::CloseSquaredBrace,
                           "Missing closing Squared Brace in computed value");
                }

                object = std::make_unique<ast::MemberExpr>(std::move(object), std::move(property), computed);
            }

            return object; // Rückgabe nicht vergessen!
        }

        // Function to parse a float Value
        float Parser::parse_float(const std::string &value)
        {
            try
            {
                return std::stof(value);
            }
            catch (...)
            {
                return 0;
            }
        }

        // Funktion to parse a primary Expression
        std::unique_ptr<fling::ast::Expr> Parser::parse_primary_expr()
        {
            auto tk = this->at();

            switch (tk.type)
            {

            // Identifier Type
            case fling::lexer::TokenType::Identifier:
            {
                // eat() returns the actual token and removes it
                auto token = this->eat();

                auto id = std::make_unique<fling::ast::Identifier>();
                id->symbol = token.value; // token.value is the identifier text
                return id;
            }

            // Number Type
            case fling::lexer::TokenType::Number:
            {
                auto id = std::make_unique<fling::ast::NumericLiteral>();
                id->value = parse_float(this->eat().value);
                return id;
            }

            // String Type
            case fling::lexer::TokenType::String:
            {
                auto str = std::make_unique<fling::ast::StringLiteral>(this->eat().value);

                // Null Check
                assert(str != nullptr);

                return str;
            }

            // Array Literal
            case fling::lexer::TokenType::OpenSquaredBrace:
            {
                this->eat();
                auto arr = std::make_unique<fling::ast::ArrayLiteral>();

                while (this->at().type != lexer::TokenType::CloseSquaredBrace)
                {
                    arr->elements.push_back(std::move(parse_expr()));

                    if (this->at().type == lexer::TokenType::Comma)
                    {
                        this->eat();
                        continue;
                    }

                    break;
                }

                this->expect(
                    fling::lexer::TokenType::CloseSquaredBrace,
                    "Expected closing square brace after array literal");

                return arr;
            }

            // Opening Parenthesis Type
            case fling::lexer::TokenType::OpenParen:
            {
                this->eat(); // Eat the opening Token
                auto expr = this->parse_expr();

                this->expect(
                    fling::lexer::TokenType::CloseParen,
                    "Unexpected Token found inside parenthesised expression. Expected closing parenthesis"); // Eat the closing Parenthesis
                
                // Null Check
                assert(expr != nullptr);

                return expr;                                                                                 // Return the inner Expression
            }

            // Open Curly Brace {
            case fling::lexer::TokenType::OpenCurlyBrace:
            {
                return this->parse_object_expr();
            }

            // Default Type for Unexpected Tokens
            default:
            {
                cout << "Unexpected Token found during Parsing: "
                     << this->at() << endl;
                
                // Remove the Error
                this->eat();

                // Return Nullpointer
                return nullptr;
            }
            }

            // End
            return nullptr;
        }

        //
        //
        // Orders of Prescidence
        // Assignment Expr
        // Member Expr
        // Function Call
        // Logical Expr
        // Comparssion Expr
        // Aditive Expr
        // Multiplicitave Expr
        // Unary Expr
        // Primary Expr
        //
        //

        // Function to produec AST
        fling::ast::Program Parser::produceAST(const std::string &sourceCode)
        {
            this->tokens = fling::lexer::tokenize(sourceCode);

            fling::ast::Program program;

            while (this->not_eof())
            {
                auto stmt = parse_stmt();
                if (stmt)
                {
                    program.body.push_back(std::move(stmt));
                }
            }

            return program;
        }
    } // namespace parser
} // namespace fling

// parser.hpp

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <cassert>

#include "lexer.hpp"
#include "ast.hpp"


namespace fling
{
    namespace parser
    {

        class Parser
        {
        private:
            /**
             * Vector Array for the List of Tokens
             */
            std::vector<fling::lexer::Token> tokens;

            /**
             * Function to check if the type of the current Token is equal to Eof
             *
             * @return Boolean for current tokentype and Eof Type
             */
            bool not_eof();

            /**
             * Parser function to get the current Token
             *
             * @return current Token
             */
            fling::lexer::Token at();

            /**
             * Parser function to get the current Token and remove it
             *
             * @return current Token
             */
            fling::lexer::Token eat();

            /**
             * Function to expect a Token of a specific Type
             *
             * @param TokenType which should be expected
             * @param string for any additional Error Message
             * @return Returns the expected Token
             */
            fling::lexer::Token expect(fling::lexer::TokenType type, const std::string& context);

            /**
             * Function to parse a statement
             *
             * @return Returns a statement from the source Code
             */
            std::unique_ptr<fling::ast::Stmt> parse_stmt();

            // Parse a statement between { }
            std::unique_ptr<fling::ast::Stmt> parse_stmt_block();

            // Parse if Statement
            std::unique_ptr<fling::ast::Stmt> parse_if_statement();

            // Parse While statement
            std::unique_ptr<fling::ast::Stmt> parse_while_statement();

            // Function to parse a Function Declaration
            std::unique_ptr<fling::ast::Stmt> parse_fn_declaration();

            /**
             * Function to parse a Variable Deklaration
             */
            std::unique_ptr<fling::ast::Stmt> parse_var_declaration();

            /**
             * Function to parse an expression
             *
             * @return Returns a Expression from the current Token
             */
            std::unique_ptr<fling::ast::Expr> parse_expr();

            // &&, ||
            std::unique_ptr<fling::ast::Expr> parse_logical_expr();

            // <, > == etc
            std::unique_ptr<fling::ast::Expr> parse_comparison_expr();

            /**
             * Function to parse an Assignment Expression
             *
             * @return Returns an Assignment Expression from the current Token
			 */
            std::unique_ptr<fling::ast::Expr> parse_assignment_expr();

            // Function to parse an Object Expression
            std::unique_ptr<fling::ast::Expr> parse_object_expr();

            /**
             * Function to parse an additive Expression
             *
             * @return an Additive Expression from the current Token
             */
            std::unique_ptr<fling::ast::Expr> parse_additive_expr();

            // Unary Expression !true, -x
            std::unique_ptr<fling::ast::Expr> parse_unary_expr();

            /**
             * Function to parse a multiplicative Expression
             *
             * @return Returns a Multiplicative Expression from the current Token
             */
            std::unique_ptr<fling::ast::Expr> parse_multiplicitave_expr();

			// Function to parse a Call Member Expression
            std::unique_ptr<fling::ast::Expr> parse_call_member_expr();

			// Function to parse a Call Expression
            std::unique_ptr<fling::ast::Expr> parse_call_expr(
                std::unique_ptr<fling::ast::Expr> caller);

			// Function to parse arguments for a Call Expression
			std::vector<std::unique_ptr<fling::ast::Expr>> parse_agrs();

            // Function to parse the Argument List
            std::vector<std::unique_ptr<fling::ast::Expr>> parse_argument_list();

			// Function to parse a Member Expression
			std::unique_ptr<fling::ast::Expr> parse_member_expr();

            /**
             * Function to parse a float Value and convert it to a String
             *
             * @param std::string as a Float Value or Basic Number
             * @return Returns an Integer from the input string
             */
            float parse_float(const std::string &value);

            /**
             * Function to parse a primary expression from the current Token
             *
             * @return Returns a primary Expression
             */
            std::unique_ptr<fling::ast::Expr> parse_primary_expr();

        public:
            /**
             * Generate an AST Program from source code
             *
             * @param std::string source Code
             * @return Program form the source Code
             */
            fling::ast::Program produceAST(const std::string &sourceCode);
        }; // Class Parser

    } // namespace parser
} // namespace fling

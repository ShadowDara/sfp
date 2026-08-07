/**
 * @file ast.hpp
 * @brief Abstract Syntax Tree (AST) Definition for the Fling Language
 * 
 * This module defines the structure of the Abstract Syntax Tree, which is an
 * intermediate representation of the source code created after parsing.
 * 
 * COMPILATION PIPELINE:
 * 1. Source Code ──(Lexer)──> Tokens
 * 2. Tokens ──(Parser)──> AST ──(This File)
 * 3. AST ──(Interpreter)──> Runtime Values
 * 
 * The AST is a tree-like structure where:
 * - Each node represents a language construct (statement or expression)
 * - Child nodes represent components of that construct
 * - The interpreter walks this tree to execute the program
 * 
 * HIERARCHY:
 * - Stmt (base class for all statements and expressions)
 *   ├─ Program (root node containing all statements)
 *   ├─ Expression nodes (NumericLiteral, StringLiteral, Identifier, etc.)
 *   ├─ Statement nodes (VarDeclaration, FunctionDeclaration, etc.)
 *   └─ Composite nodes (BinaryExpr, CallExpr, MemberExpr, etc.)
 */

// ast.hpp

/**
 * Header file for the Abstract Syntax Tree for the Language
 */

#pragma once

#include <string_view>
#include <vector>
#include <string>
#include <ostream>
#include <memory>

namespace fling
{
    namespace ast
    {
        /**
         * Function to prodruce an Indent String
         *
         * @param int for the indent counter
         * @return std::string for indents
         */
        static std::string indentStr(int indent)
        {
            return std::string(indent, ' ');
        }

        /**
         * @enum NodeType
         * @brief Enumeration of all possible AST node types in the Fling language
         * 
         * Each NodeType represents a different language construct. The type is stored
         * in every AST node to allow the interpreter to know how to handle it.
         * 
         * CATEGORIES:
         * 
         * STATEMENTS (Top-level constructs that produce effects):
         * - Program: Root node, contains all statements in a program
         * - VarDeclaration: Variable declaration (let/const x = ...)
         * - FunctionDeclaration: Function definition (fn name(params) { body })
         * 
         * EXPRESSIONS (Constructs that evaluate to values):
         * - Literal Expressions: Values directly in code
         *   - NumericLiteral: Number like 42, 3.14
         *   - StringLiteral: Text like "hello"
         *   - ObjectLiteral: Object {...}
         *   - ArrayLiteral: Array [...]
         * 
         * - Compound Expressions: Combine other values/expressions
         *   - Identifier: Variable/function reference
         *   - BinaryExpr: Operation like x + y, a * b
         *   - CallExpr: Function call like func(args)
         *   - MemberExpr: Property access like obj.prop or arr[0]
         *   - AssignmentExpr: Assignment like x = value
         * 
         * - Helper Types:
         *   - Property: Key-value pair in object literals
         */
        enum class NodeType
        {
            // STATEMENTS
            Program,
            VarDeclaration,
            FunctionDeckaration,
            IfStatement,
            WhileStatement,

            // EXPRESSIONS
            AssignmentExpr,
            MemberExpr,
            CallExpr,

            // Literals
            Property,
            ObjectLiteral,
            ArrayLiteral,
            StringLiteral,
            NumericLiteral,
            Identifier,
            BinaryExpr,
            UnaryExpr,
        };

        // Nodetype toString
        std::string toString(NodeType type);

        /**
         * @class Stmt
         * @brief Base class for all AST nodes (statements and expressions)
         * 
         * This is the root of the AST node hierarchy. Every node in the abstract
         * syntax tree inherits from this class and must implement:
         * - A NodeType identifier (stored in 'kind')
         * - toString() method for debugging/printing
         * - clone() method for deep copying the node
         * 
         * USAGE PATTERN:
         * The interpreter walks the AST by checking the 'kind' field and then
         * safely casting to the appropriate derived type, knowing it's safe because
         * the kind matches the actual type.
         * 
         * Example in interpreter:
         *   if (node.kind == NodeType::NumericLiteral) {
         *       auto& numNode = static_cast<const NumericLiteral&>(node);
         *       // Now we can safely access numNode.value
         *   }
         */
        struct Stmt
        {
            NodeType kind;  ///< Identifies what type of AST node this is

            explicit Stmt(NodeType kind) : kind(kind) {}
            virtual ~Stmt() = default;

            /// @brief Returns a human-readable string representation (for debugging)
            virtual std::string toString(int indent = 0) const
            {
                return "";
            }

            /// @brief Creates a deep copy of this node and all its children
            virtual std::unique_ptr<Stmt> clone() const = 0;
        };

        // Program
        struct Program : Stmt
        {
            std::vector<std::unique_ptr<Stmt>> body;

            // toString function
            std::string toString(int indent = 0) const override
            {
                std::string out = indentStr(indent) + "Program:\n";

                for (const auto &stmt : body)
                {
                    out += stmt->toString(indent + 2);
                    // Add newline After each statement
                    out += "\n";
                }

                return out;
            }

            Program() : Stmt(NodeType::Program) {} // Liste von Statements
            Program(Program &&) = default;
            Program &operator=(Program &&) = default;
            Program(const Program &) = delete;
            Program &operator=(const Program &) = delete;

            // Clone Function
            std::unique_ptr<Stmt> clone() const override
            {
                auto p = std::make_unique<Program>();

                for (const auto &stmt : body)
                {
                    p->body.push_back(stmt->clone());
                }

                return p;
            }
        };

        // Ausdruck
        struct Expr : Stmt
        {
            explicit Expr(NodeType kind) : Stmt(kind) {}

            virtual ~Expr() = default;

            // wichtig: kein clone hier implementieren
            std::unique_ptr<Stmt> clone() const override = 0;
        };

        // Assignment Expression
        //
        //
        struct AssignmentExpr : Expr
        {
            std::unique_ptr<ast::Expr> assignme;
            std::unique_ptr<ast::Expr> value;

            AssignmentExpr() : Expr(NodeType::AssignmentExpr) {}
            AssignmentExpr(AssignmentExpr &&) = default;
            AssignmentExpr &operator=(AssignmentExpr &&) = default;
            AssignmentExpr(const AssignmentExpr &) = delete;
            AssignmentExpr &operator=(const AssignmentExpr &) = delete;

            std::string toString(int indent = 0) const override
            {
                std::string out = indentStr(indent) + "AssignmentExpr:\n";

                out += indentStr(indent + 2) + "Target:\n";
                out += assignme->toString(indent + 4) + "\n";

                out += indentStr(indent + 2) + "Value:\n";
                out += value->toString(indent + 4);

                return out;
            }

            // Clone Function
            std::unique_ptr<Stmt> clone() const override
            {
                auto a = std::make_unique<AssignmentExpr>();

                a->assignme = assignme ? std::unique_ptr<Expr>(
                                             static_cast<Expr *>(assignme->clone().release()))
                                       : nullptr;

                a->value = value ? std::unique_ptr<Expr>(
                                       static_cast<Expr *>(value->clone().release()))
                                 : nullptr;

                return a;
            }
        };

        // Variable Declaration
        struct VarDeclaration : Stmt
        {
            bool constant;
            std::string identifier;
            std::unique_ptr<ast::Expr> value;

            VarDeclaration() : Stmt(NodeType::VarDeclaration) {}
            VarDeclaration(VarDeclaration &&) = default;
            VarDeclaration &operator=(VarDeclaration &&) = default;
            VarDeclaration(const VarDeclaration &) = delete;
            VarDeclaration &operator=(const VarDeclaration &) = delete;

            std::string toString(int indent = 0) const override
            {
                std::string out = indentStr(indent) + "VarDeclaration:\n";
                out += indentStr(indent + 2) + "Identifier: " + identifier + "\n";
                out += indentStr(indent + 2) + "Constant: " + std::string(constant ? "true" : "false") + "\n";

                if (value)
                {
                    out += indentStr(indent + 2) + "Value:\n";
                    out += value->toString(indent + 4);
                }

                return out;
            }

            // Clone Function
            std::unique_ptr<Stmt> clone() const override
            {
                auto v = std::make_unique<VarDeclaration>();

                v->constant = constant;
                v->identifier = identifier;

                if (value)
                    v->value = std::unique_ptr<Expr>(
                        static_cast<Expr *>(value->clone().release()));

                return v;
            }
        };

        // Function Declaration
        struct FunctionDeclaration : Stmt
        {
            std::vector<std::string> parameters;
            std::string name;
            std::vector<std::unique_ptr<ast::Stmt>> body;

            FunctionDeclaration(std::string n, std::vector<std::unique_ptr<ast::Stmt>> b,
                                std::vector<std::string> p) : Stmt(NodeType::FunctionDeckaration),
                                                              name(n), body(std::move(b)), parameters(p) {}

            std::string toString(int indent = 0) const override
            {
                /*std::string out = indentStr(indent) + "VarDeclaration:\n";
                out += indentStr(indent + 2) + "Identifier: " + identifier + "\n";
                out += indentStr(indent + 2) + "Constant: " + std::string(constant ? "true" : "false") + "\n";

                if (value)
                {
                    out += indentStr(indent + 2) + "Value:\n";
                    out += value->toString(indent + 4);
                }*/
                std::string out = "Function to string has not been set up :(";

                return out;
            }

            // Clone Function
            std::unique_ptr<Stmt> clone() const override
            {
                auto f = std::make_unique<FunctionDeclaration>(name, std::vector<std::unique_ptr<Stmt>>{}, parameters);

                for (const auto &stmt : body)
                {
                    f->body.push_back(stmt->clone());
                }

                return f;
            }
        };

        // Binary Expression
        struct BinaryExpr : Expr
        {
            std::unique_ptr<Expr> left;
            std::unique_ptr<Expr> right;
            std::string callculation_operator;

            // String-Konvertierung (optional)
            operator std::string() const
            {
                return "BinaryExpr";
            }

            // toString function
            std::string toString(int indent = 0) const override
            {
                std::string out = indentStr(indent) + "BinaryExpr:\n";
                out += indentStr(indent + 2) + "Left:\n";
                out += left->toString(indent + 4) + "\n";
                out += indentStr(indent + 2) + "Right:\n";
                out += right->toString(indent + 4) + "\n";
                out += indentStr(indent + 2) + "Binary Operator:\n";
                out += indentStr(indent + 4) + callculation_operator;
                return out;
            }

            BinaryExpr() : Expr(ast::NodeType::BinaryExpr) {}
            BinaryExpr(BinaryExpr &&) = default;
            BinaryExpr &operator=(BinaryExpr &&) = default;
            BinaryExpr(const BinaryExpr &) = delete;
            BinaryExpr &operator=(const BinaryExpr &) = delete;

            // Clone Function
            std::unique_ptr<Stmt> clone() const override
            {
                auto b = std::make_unique<BinaryExpr>();

                b->callculation_operator = callculation_operator;

                if (left)
                    b->left = std::unique_ptr<Expr>(
                        static_cast<Expr*>(left->clone().release()));

                if (right)
                    b->right = std::unique_ptr<Expr>(
                        static_cast<Expr*>(right->clone().release()));

                return b;
            }
        };

        // Unary Expression
        struct UnaryExpr : Expr
        {
            std::string op;                // "!" oder "-"
            std::unique_ptr<Expr> operand;   // Ausdruck rechts davon

            UnaryExpr() : Expr(NodeType::UnaryExpr) {}

            std::string toString(int indent = 0) const override
            {
                std::string out = indentStr(indent) + "UnaryExpr:\n";
                out += indentStr(indent + 2) + "Operator: " + op + "\n";
                out += indentStr(indent + 2) + "Right:\n";
                out += operand ? operand->toString(indent + 4) : "null";
                return out;
            }

            std::unique_ptr<Stmt> clone() const override
            {
                auto u = std::make_unique<UnaryExpr>();
                u->op = op;

                if (operand)
                    u->operand = std::unique_ptr<Expr>(
                        static_cast<Expr*>(operand->clone().release()));

                return u;
            }
        };

        // Call Expression
        struct CallExpr : Expr
        {
            std::vector<std::unique_ptr<Expr>> agrs;
            std::unique_ptr<Expr> caller;

            //// String-Konvertierung (optional)
            // operator std::string() const
            //{
            //     return "BinaryExpr";
            // }

            //// toString function
            // std::string toString(int indent = 0) const override
            //{
            //     std::string out = indentStr(indent) + "BinaryExpr:\n";
            //     out += indentStr(indent + 2) + "Left:\n";
            //     out += left->toString(indent + 4) + "\n";
            //     out += indentStr(indent + 2) + "Right:\n";
            //     out += right->toString(indent + 4) + "\n";
            //     out += indentStr(indent + 2) + "Binary Operator:\n";
            //     out += indentStr(indent + 4) + callculation_operator;
            //     return out;
            // }

            CallExpr(std::unique_ptr<Expr> c,
                     std::vector<std::unique_ptr<fling::ast::Expr>> args) : Expr(ast::NodeType::CallExpr), caller(std::move(c)),
                                                                            agrs(std::move(args)) {}

            CallExpr(CallExpr &&) = default;
            CallExpr &operator=(CallExpr &&) = default;
            CallExpr(const CallExpr &) = delete;
            CallExpr &operator=(const CallExpr &) = delete;

            std::string toString(int indent = 0) const override
            {
                std::string out = indentStr(indent) + "CallExpr:\n";

                out += indentStr(indent + 2) + "Caller:\n";
                out += caller->toString(indent + 4) + "\n";

                out += indentStr(indent + 2) + "Args:\n";
                for (const auto &arg : agrs)
                {
                    out += arg->toString(indent + 4) + "\n"; // <- Use '->'
                }

                return out;
            }

            // Clone Function
            std::unique_ptr<Stmt> clone() const override
            {
                std::vector<std::unique_ptr<Expr>> cloned_args;
                for (const auto &arg : agrs)
                {
                    cloned_args.push_back(std::unique_ptr<Expr>(
                        static_cast<Expr *>(arg->clone().release())));
                }
                return std::make_unique<CallExpr>(
                    caller ? std::unique_ptr<Expr>(
                                 static_cast<Expr *>(caller->clone().release()))
                           : nullptr,
                    std::move(cloned_args));
            }
        };

        // Member Expression
        struct MemberExpr : Expr
        {
            std::unique_ptr<Expr> object;
            std::unique_ptr<Expr> property;
            bool computed; // true for obj[prop], false for obj.prop

            //// String-Konvertierung (optional)
            // operator std::string() const
            //{
            //     return "BinaryExpr";
            // }

            //// toString function
            // std::string toString(int indent = 0) const override
            //{
            //     std::string out = indentStr(indent) + "BinaryExpr:\n";
            //     out += indentStr(indent + 2) + "Left:\n";
            //     out += left->toString(indent + 4) + "\n";
            //     out += indentStr(indent + 2) + "Right:\n";
            //     out += right->toString(indent + 4) + "\n";
            //     out += indentStr(indent + 2) + "Binary Operator:\n";
            //     out += indentStr(indent + 4) + callculation_operator;
            //     return out;
            // }

            MemberExpr(std::unique_ptr<Expr> o,
                       std::unique_ptr<Expr> p,
                       bool c) : Expr(ast::NodeType::MemberExpr),
                                 object(std::move(o)), property(std::move(p)),
                                 computed(c) {}

            MemberExpr(MemberExpr &&) = default;
            MemberExpr &operator=(MemberExpr &&) = default;
            MemberExpr(const MemberExpr &) = delete;
            MemberExpr &operator=(const MemberExpr &) = delete;

            std::string toString(int indent = 0) const override
            {
                std::string out = indentStr(indent) + "MemberExpr:\n";

                out += indentStr(indent + 2) + "Object:\n";
                out += object->toString(indent + 4) + "\n";

                out += indentStr(indent + 2) + "Property:\n";
                out += property->toString(indent + 4) + "\n";

                out += indentStr(indent + 2) + "Computed: ";
                out += (computed ? "true" : "false");

                return out;
            }

            // Clone Function
            std::unique_ptr<Stmt> clone() const override
            {
                return std::make_unique<MemberExpr>(
                    object ? std::unique_ptr<Expr>(
                                 static_cast<Expr *>(object->clone().release()))
                           : nullptr,
                    property ? std::unique_ptr<Expr>(
                                   static_cast<Expr *>(property->clone().release()))
                             : nullptr,
                    computed);
            }
        };

        // Identifier
        struct Identifier : Expr
        {
            std::string symbol;

            // toString function
            std::string toString(int indent = 0) const override
            {
                return indentStr(indent) + "Identifier(" + symbol + ")";
            }

            // Standardkonstruktor
            Identifier() : Expr(NodeType::Identifier), symbol("") {}

            // Parameter-Konstruktor
            Identifier(std::string s)
                : Expr(NodeType::Identifier), symbol(std::move(s))
            {
            }

            // Clone Function
            std::unique_ptr<Stmt> clone() const override
            {
                return std::make_unique<Identifier>(*this);
            }
        };

        // If Statement
        struct IfStatement : Stmt {
            std::unique_ptr<Expr> condition;
            std::unique_ptr<Stmt> thenBranch;
            std::unique_ptr<Stmt> elseBranch; // optional, kann nullptr sein

            IfStatement()
                : Stmt(NodeType::IfStatement) {}

            std::string toString(int indent = 0) const override {
                std::string out = indentStr(indent) + "IfStatement:\n";
                out += indentStr(indent + 2) + "Condition:\n";
                out += condition ? condition->toString(indent + 4) + "\n" : indentStr(indent + 4) + "null\n";
                out += indentStr(indent + 2) + "Then:\n";
                out += thenBranch ? thenBranch->toString(indent + 4) + "\n" : indentStr(indent + 4) + "null\n";
                out += indentStr(indent + 2) + "Else:\n";
                out += elseBranch ? elseBranch->toString(indent + 4) : indentStr(indent + 4) + "null";
                return out;
            }

            std::unique_ptr<Stmt> clone() const override {
                auto node = std::make_unique<IfStatement>();
                node->condition = condition ? std::unique_ptr<Expr>(static_cast<Expr*>(condition->clone().release())) : nullptr;
                node->thenBranch = thenBranch ? thenBranch->clone() : nullptr;
                node->elseBranch = elseBranch ? elseBranch->clone() : nullptr;
                return node;
            }
        };

        // While Statement
        struct WhileStatement : Stmt
        {
            std::unique_ptr<Expr> condition;
            std::unique_ptr<Stmt> body;

            WhileStatement()
                : Stmt(NodeType::WhileStatement) {}

            std::string toString(int indent = 0) const override {
                std::string out = indentStr(indent) + "WhileStatement:\n";

                out += indentStr(indent + 2) + "Condition:\n";
                out += condition
                    ? condition->toString(indent + 4) + "\n"
                    : indentStr(indent + 4) + "null\n";

                out += indentStr(indent + 2) + "Body:\n";
                out += body
                    ? body->toString(indent + 4)
                    : indentStr(indent + 4) + "null";

                return out;
            }

            std::unique_ptr<Stmt> clone() const override {
                auto node = std::make_unique<WhileStatement>();

                node->condition = condition
                    ? std::unique_ptr<Expr>(
                        static_cast<Expr*>(condition->clone().release()))
                    : nullptr;

                node->body = body
                    ? body->clone()
                    : nullptr;

                return node;
            }
        };

        // Numerisches Literal
        struct NumericLiteral : Expr
        {
            float value;

            // toString Function
            std::string toString(int indent = 0) const override
            {
                return indentStr(indent) + "NumericLiteral(" + std::to_string(value) + ")";
            }

            // Standardkonstruktor
            NumericLiteral() : Expr(NodeType::NumericLiteral), value(0) {}

            // Parameter-Konstruktor
            NumericLiteral(int v) : Expr(NodeType::NumericLiteral), value(v) {}

            // Clone Function
            std::unique_ptr<Stmt> clone() const override
            {
                return std::make_unique<NumericLiteral>(*this);
            }
        };

        // Property für Objektliteral
        struct Property : Expr
        {
            std::string key;

            // Optional Value!
            std::unique_ptr<ast::Expr> value;

            // toString function
            std::string toString(int indent = 0) const override
            {
                std::string out = indentStr(indent) + "Property:\n";
                out += indentStr(indent + 2) + "Key: " + key + "\n";
                out += indentStr(indent + 2) + "Value:\n";
                out += value->toString(indent + 4);
                return out;
            }

            // Konstruktor
            Property() : Expr(NodeType::Property) {}

            Property(Property &&) = default;

            Property &operator=(Property &&) = default;

            Property(const Property &) = delete;

            Property &operator=(const Property &) = delete;

            // Clone Function
            std::unique_ptr<Stmt> clone() const override
            {
                auto p = std::make_unique<Property>();
                p->key = key;
                p->value = value ? std::unique_ptr<Expr>(
                                       static_cast<Expr *>(value->clone().release()))
                                 : nullptr;
                return p;
            }
        };

        // Array literal
        struct ArrayLiteral : Expr
        {
            std::vector<std::unique_ptr<Expr>> elements;

            std::string toString(int indent = 0) const override
            {
                std::string out = "[\n";

                for (size_t i = 0; i < elements.size(); ++i)
                {
                    out += indentStr(indent + 2);
                    out += elements[i]->toString(indent + 2);
                    if (i < elements.size() - 1)
                        out += ",";
                    out += "\n";
                }

                out += indentStr(indent) + "]";
                return out;
            }

            // Konstruktor
            ArrayLiteral() : Expr(NodeType::ArrayLiteral) {}

            // Clone Function
            std::unique_ptr<Stmt> clone() const override
            {
                auto arr = std::make_unique<ArrayLiteral>();

                for (const auto &element : elements)
                {
                    arr->elements.push_back(
                        std::unique_ptr<Expr>(
                            static_cast<Expr *>(element->clone().release())));
                }

                return arr;
            }
        };

        // String literal
        struct StringLiteral : Expr
        {
            std::string value;

            std::string toString(int indent = 0) const override
            {
                return "\"" + value + "\"";
            }

            // Konstruktor
            StringLiteral(std::string val) : Expr(NodeType::StringLiteral), value(std::move(val)) {}

            // Clone Function
            std::unique_ptr<Stmt> clone() const override
            {
                return std::make_unique<StringLiteral>(value);
            }
        };

        // Objektliteral
        struct ObjectLiteral : Expr
        {
            std::vector<std::unique_ptr<Property>> properties;

            std::string toString(int indent = 0) const override
            {
                std::string out = "{\n";

                for (size_t i = 0; i < properties.size(); ++i)
                {
                    const auto &prop = properties[i];

                    out += indentStr(indent + 2);
                    out += prop->key + ": ";

                    if (prop->value)
                        out += prop->value->toString(indent + 2);
                    else
                        out += "null";

                    if (i < properties.size() - 1)
                        out += ",";

                    out += "\n";
                }

                out += indentStr(indent) + "}";

                return out;
            }

            // Konstruktor
            ObjectLiteral() : Expr(NodeType::ObjectLiteral) {}

            // Clone Function
            std::unique_ptr<Stmt> clone() const override
            {
                auto obj = std::make_unique<ObjectLiteral>();

                for (const auto &prop : properties)
                {
                    obj->properties.push_back(
                        std::unique_ptr<Property>(
                            static_cast<Property *>(prop->clone().release())));
                }

                return obj;
            }
        };

        // for toString
        inline std::ostream &operator<<(std::ostream &os, const fling::ast::Stmt &stmt)
        {
            os << stmt.toString();
            return os;
        }

        // for toString
        inline std::ostream &operator<<(std::ostream &os, const fling::ast::Program &program)
        {
            os << program.toString();
            return os;
        }

    } // namespace ast
} // namespace fling

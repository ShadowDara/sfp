#include "ast.hpp"

// AST Nodetype toString
std::string fling::ast::toString(NodeType type)
{
    switch (type)
    {
    case NodeType::Program: return "Program";
    case NodeType::VarDeclaration: return "VarDeclaration";
    case NodeType::FunctionDeckaration: return "FunctionDeckaration";
    case NodeType::IfStatement: return "IfStatement";
    case NodeType::WhileStatement: return "WhileStatement";

    case NodeType::AssignmentExpr: return "AssignmentExpr";
    case NodeType::MemberExpr: return "MemberExpr";
    case NodeType::CallExpr: return "CallExpr";

    case NodeType::Property: return "Property";
    case NodeType::ObjectLiteral: return "ObjectLiteral";
    case NodeType::ArrayLiteral: return "ArrayLiteral";
    case NodeType::StringLiteral: return "StringLiteral";
    case NodeType::NumericLiteral: return "NumericLiteral";
    case NodeType::Identifier: return "Identifier";
    case NodeType::BinaryExpr: return "BinaryExpr";
    case NodeType::UnaryExpr: return "UnaryExpr";

    // return The Number to count in the ENUM
    default: return "Unknown AST Node Type: " + static_cast<int>(type);
    }
}
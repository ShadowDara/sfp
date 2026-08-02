// interpreter.cpp

#include "interpreter.hpp"

using namespace std;
using namespace fling;
using namespace fling::util;
using namespace fling::ast;
using namespace fling::runtime;
using namespace fling::runtime::envirment;
// using namespace fling::runtime::eval;

namespace fling
{
    namespace runtime
    {
        // Function to evaluate Source Code
        runtime::RuntimeVal evaluate(
            const ast::Stmt &astNode,
            std::shared_ptr<runtime::envirment::Environment> env)
        {
            switch (astNode.kind)
            {
            // If Statement
            case ast::NodeType::IfStatement:
            {
                auto &ifNode = static_cast<const ast::IfStatement &>(astNode);
                auto condVal = evaluate(*ifNode.condition, env);
                if (condVal.isTruthy())
                {
                    return evaluate(*ifNode.thenBranch, env);
                }
                else if (ifNode.elseBranch)
                {
                    return evaluate(*ifNode.elseBranch, env);
                }
                else
                {
                    return RuntimeVal::Null();
                }
            }

            // While Statement
            // 
            // The Difficult Part is that every while iteration needs a new subenvirment
            // from the parent envirment so that a variable which is declared in the while
            // will be undeclared after the while loop.
            //
            case ast::NodeType::WhileStatement:
            {
                auto &whileNode = static_cast<const ast::WhileStatement &>(astNode);

                while (true)
                {
                    auto condVal = evaluate(*whileNode.condition, env);
                    if (!condVal.isTruthy())
                        break;

                    auto iterationEnv = std::make_shared<envirment::Environment>(env);

                    evaluate(*whileNode.body, iterationEnv);
                }

                return RuntimeVal::Null();
            }

            // Number Value
            case ast::NodeType::NumericLiteral:
            {
                auto &numNode = static_cast<const ast::NumericLiteral &>(astNode);
                return RuntimeVal(static_cast<float>(numNode.value));
            }

            // Identifier
            case ast::NodeType::Identifier:
            {
                auto &identNode = static_cast<const ast::Identifier &>(astNode);
                return env->lookupVar(identNode.symbol);
            }

            // Object Literal
            case ast::NodeType::ObjectLiteral:
            {
                auto &objNode = static_cast<const ast::ObjectLiteral &>(astNode);
                return eval::evaluate_object_expr(objNode, env);
            }

            // Array Literal
            case ast::NodeType::ArrayLiteral:
            {
                auto &arrNode = static_cast<const ast::ArrayLiteral &>(astNode);
                std::vector<RuntimeVal> elements;
                for (const auto &elem : arrNode.elements)
                {
                    elements.push_back(evaluate(*elem, env));
                }
                return RuntimeVal(elements);
            }

            // String Literal
            case ast::NodeType::StringLiteral:
            {
                auto &strNode = static_cast<const ast::StringLiteral &>(astNode);
                return RuntimeVal::String(strNode.value);
            }

            // Member Expression
            case ast::NodeType::MemberExpr:
            {
                auto &memExpr = static_cast<const ast::MemberExpr &>(astNode);
                auto objVal = evaluate(*memExpr.object, env);

                // Array
                if (objVal.type == RuntimeVal::Type::Array)
                {
                    if (memExpr.computed)
                    {
                        if (!memExpr.property)
                        {
                            return RuntimeVal::Null();
                        }
                        auto indexVal = evaluate(*memExpr.property, env);
                        if (indexVal.type != RuntimeVal::Type::Number)
                        {
                            return RuntimeVal::Null();
                        }
                        int index = static_cast<int>(indexVal.number);
                        if (index < 0 || index >= static_cast<int>(objVal.elements.size()))
                        {
                            return RuntimeVal::Null();
                        }
                        return objVal.elements[index];
                    }
                    else
                    {
                        auto propIdent = static_cast<ast::Identifier *>(memExpr.property.get());
                        if (!propIdent)
                        {
                            return RuntimeVal::Null();
                        }
                        if (propIdent->symbol == "length")
                        {
                            return RuntimeVal(static_cast<float>(objVal.elements.size()));
                        }
                        return RuntimeVal::Null();
                    }
                }

                // String
                else if (objVal.type == RuntimeVal::Type::String)
                {
                    auto propIdent = static_cast<ast::Identifier *>(memExpr.property.get());
                    if (!propIdent)
                    {
                        return RuntimeVal::Null();
                    }
                    if (propIdent->symbol == "length")
                    {
                        return RuntimeVal(static_cast<float>(objVal.str.length()));
                    }
                    return RuntimeVal::Null();
                }

                // Object
                else if (objVal.type == RuntimeVal::Type::Object)
                {
                    std::string propertyKey;
                    if (memExpr.computed)
                    {
                        if (!memExpr.property)
                        {
                            return RuntimeVal::Null();
                        }
                        auto propVal = evaluate(*memExpr.property, env);
                        propertyKey = propVal.toString();
                    }
                    else
                    {
                        auto propIdent = static_cast<ast::Identifier *>(memExpr.property.get());
                        if (!propIdent)
                        {
                            return RuntimeVal::Null();
                        }
                        propertyKey = propIdent->symbol;
                    }

                    auto it = objVal.properties.find(propertyKey);
                    if (it == objVal.properties.end() || !it->second)
                    {
                        return RuntimeVal::Null();
                    }

                    return *it->second;
                }

                // Null
                else
                {
                    return RuntimeVal::Null();
                }
            }

            // Call Expression
            case ast::NodeType::CallExpr:
            {
                auto &objNode = static_cast<const ast::CallExpr &>(astNode);
                return eval::evaluate_call_expr(objNode, env);
            }

            // Assignment Expression
            case ast::NodeType::AssignmentExpr:
            {
                auto &assignexpr = static_cast<const ast::AssignmentExpr &>(astNode); // ⚡ Referenz!
                return eval::evaluate_assignment_expr(assignexpr, env);
            }

            // Binary Expression
            case ast::NodeType::BinaryExpr:
            {
                auto &binNode = static_cast<const ast::BinaryExpr &>(astNode);

                // Null Check
                assert(binNode.left != nullptr);
                assert(binNode.right != nullptr);

                // BinNode Right is null here, but why
                // IDK

                return eval::evaluate_binary_expr(binNode, env);
            }

            // Unary Expression
            case ast::NodeType::UnaryExpr:
            {
                auto &unNode = static_cast<const ast::UnaryExpr &>(astNode);

                auto val = evaluate(*unNode.operand, env);

                if (unNode.op == "-")
                {
                    if (val.type != RuntimeVal::Type::Number)
                        return RuntimeVal::Null();

                    return RuntimeVal(-val.number);
                }

                if (unNode.op == "!")
                {
                    return RuntimeVal(!val.isTruthy());
                }

                return RuntimeVal::Null();
            }

            // Program Node
            case ast::NodeType::Program:
            {
                auto &prog = static_cast<const ast::Program &>(astNode); // Reference
                return eval::evaluate_program(prog, env);
            }

            // Handle Statement Nodes
            case ast::NodeType::VarDeclaration:
            {
                auto &declNode = static_cast<const ast::VarDeclaration &>(astNode);
                // auto value = declNode.value ? evaluate(*declNode.value, env) : RuntimeVal();
                // return env.declareVar(declNode.identifier, value, declNode.constant);
                return eval::evaluate_var_declaration(declNode, env);
            }

            // Handle Function Declaration
            case ast::NodeType::FunctionDeckaration:
            {
                auto &declNode = static_cast<const ast::FunctionDeclaration &>(astNode);
                return eval::evaluate_fn_declaration(declNode, env);
            }

            // Error Fallback
            default:
            {
                cout << "Statement has not been setup for Interpretation: "
                     << ast::toString(astNode.kind) << endl;

                // Null Value
                return RuntimeVal::Null();
            }
            }
        }
    } // namespace runtime
} // namespace fling

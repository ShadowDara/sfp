#include "expressions.hpp"

using namespace fling;
using namespace fling::runtime;
using namespace fling::util;

// Function to Evaluate / Calculate the 2 Numbers
runtime::RuntimeVal fling::runtime::eval::evaluate_numeric_binary_expr(
    const RuntimeVal& lhs,
    const RuntimeVal& rhs,
    std::string callculation_operator,
    std::shared_ptr<runtime::envirment::Environment> env)
{
    float result = 0.0f;

    // Additon
    if (callculation_operator == "+")
    {
        result = lhs.number + rhs.number;
    }
    // Subtraction
    else if (callculation_operator == "-")
    {
        result = lhs.number - rhs.number;
    }
    // Multiplikation
    else if (callculation_operator == "*")
    {
        result = lhs.number * rhs.number;
    }
    // Division
    else if (callculation_operator == "/")
    {
        // Division durch 0 vermeiden
        result = (rhs.number != 0) ? lhs.number / rhs.number : 0.0f;
    }
    // Module
    else if (callculation_operator == "%")
    {
        result = static_cast<float>(toInt(rhs.number) % toInt(lhs.number));
    }
    // Error
    else
    {
        // Unbekannter Operator, Fehlerbehandlung
        result = 0.0f;
    }

    return runtime::RuntimeVal(result);
}

// Function to evaluate a Binary Expression
runtime::RuntimeVal fling::runtime::eval::evaluate_binary_expr(
    const ast::BinaryExpr &binop,
    std::shared_ptr<runtime::envirment::Environment> env)
{
    // They should not be null
    assert(binop.right != nullptr);
    assert(binop.left != nullptr);

    // Referrencing them!
    auto lhs = evaluate(*binop.left, env);
    auto rhs = evaluate(*binop.right, env);

    // Handle comparison operators
    if (binop.callculation_operator == "==")
    {
        // Type-safe equality comparison
        if (lhs.type != rhs.type)
            return RuntimeVal::Boolean(false);
        
        switch (lhs.type)
        {
            case RuntimeVal::Type::Null:
                return RuntimeVal::Boolean(true);
            case RuntimeVal::Type::Number:
                return RuntimeVal::Boolean(lhs.number == rhs.number);
            case RuntimeVal::Type::String:
                return RuntimeVal::Boolean(lhs.str == rhs.str);
            case RuntimeVal::Type::Boolean:
                return RuntimeVal::Boolean(lhs.bvalue == rhs.bvalue);
            default:
                return RuntimeVal::Boolean(false);
        }
    }
    
    if (binop.callculation_operator == "!=")
    {
        // Type-safe inequality comparison
        if (lhs.type != rhs.type)
            return RuntimeVal::Boolean(true);
        
        switch (lhs.type)
        {
            case RuntimeVal::Type::Null:
                return RuntimeVal::Boolean(false);
            case RuntimeVal::Type::Number:
                return RuntimeVal::Boolean(lhs.number != rhs.number);
            case RuntimeVal::Type::String:
                return RuntimeVal::Boolean(lhs.str != rhs.str);
            case RuntimeVal::Type::Boolean:
                return RuntimeVal::Boolean(lhs.bvalue != rhs.bvalue);
            default:
                return RuntimeVal::Boolean(true);
        }
    }
    
    // Handle logical operators
    if (binop.callculation_operator == "&&")
    {
        return RuntimeVal::Boolean(lhs.isTruthy() && rhs.isTruthy());
    }
    
    if (binop.callculation_operator == "||")
    {
        return RuntimeVal::Boolean(lhs.isTruthy() || rhs.isTruthy());
    }

    if (binop.callculation_operator == "<")
        return RuntimeVal::Boolean(lhs.number < rhs.number);

    if (binop.callculation_operator == ">")
        return RuntimeVal::Boolean(lhs.number > rhs.number);

    if (binop.callculation_operator == "<=")
        return RuntimeVal::Boolean(lhs.number <= rhs.number);

    if (binop.callculation_operator == ">=")
        return RuntimeVal::Boolean(lhs.number >= rhs.number);

    if (lhs.type == runtime::RuntimeVal::Type::Number && rhs.type == runtime::RuntimeVal::Type::Number)
    {
        return evaluate_numeric_binary_expr(
            lhs, rhs, binop.callculation_operator, env);
    }

    // One or both are Null
    return runtime::RuntimeVal::Null();
}

// Function to evaluate an Identifier
runtime::RuntimeVal fling::runtime::eval::evaluate_identifier(
    const ast::Identifier &ident,
    std::shared_ptr<runtime::envirment::Environment> env)
{
    return env->lookupVar(ident.symbol);
}

// Function to evaluate an Assignment Expression
runtime::RuntimeVal fling::runtime::eval::evaluate_assignment_expr(
    const ast::AssignmentExpr &node,
    std::shared_ptr<runtime::envirment::Environment> env)
{
    if (node.assignme->kind != ast::NodeType::Identifier)
    {
        std::cerr << "Left-hand side of assignment must be an identifier."
                  << std::endl;
    }

    auto varName = static_cast<ast::Identifier *>(node.assignme.get())->symbol;

    // Use a Reference instead of a unique Pointer
    return env->assignVar(varName, evaluate(*node.value, env));
}

// Function to evaluate an Object Literal
runtime::RuntimeVal fling::runtime::eval::evaluate_object_expr(
    const ast::ObjectLiteral &node,
    std::shared_ptr<runtime::envirment::Environment> env)
{
    auto objectValue = runtime::RuntimeVal::Object();

    for (const auto &prop : node.properties)
    {
        auto key = prop->key;
        auto value = prop->value ? evaluate(*prop->value, env) : env->lookupVar(key);

        objectValue.properties[key] = std::make_unique<RuntimeVal>(std::move(value));
    }

    return objectValue;
}

runtime::RuntimeVal fling::runtime::eval::evaluate_call_expr(
    const ast::CallExpr &expr,
    std::shared_ptr<runtime::envirment::Environment> env)
{
    std::vector<RuntimeVal> evaluatedArgs;
    evaluatedArgs.reserve(expr.agrs.size()); // optional, spart Reallocs

    for (const auto &arg : expr.agrs)
    {
        evaluatedArgs.push_back(evaluate(*arg, env)); // *arg, weil unique_ptr
    }

    auto fn = evaluate(*expr.caller, env);

    if (fn.type == RuntimeVal::Type::Native_FnValue)
    {
        auto result = fn.call(evaluatedArgs, env);
        return result;
    }
    
    if (fn.type == RuntimeVal::Type::FnValue)
    {
        auto func = std::move(fn);
        auto scope = std::make_shared<envirment::Environment>(func.declaration);
        // Create a new scope for the function call
        
        // Create the Variables for the Parameters
        for (size_t i = 0; i < func.parameters.size(); ++i)
        {
            auto paramName = func.parameters[i];
            RuntimeVal argValue = (i < evaluatedArgs.size()) ? std::move(evaluatedArgs[i]) : RuntimeVal::Null();
            scope->declareVar(paramName, std::move(argValue), false);
        }

        RuntimeVal returnValue = RuntimeVal::Null();
        for (const auto& stmt : func.body)
        {
            returnValue = evaluate(*stmt, scope);
        }

        if (returnValue.type == RuntimeVal::Type::Null && scope->hasVar("result"))
        {
            return scope->lookupVar("result");
        }

        return returnValue;
    }

    std::cerr << "Can not call value that is not a Native Function: "
                  << fn.toString() << std::endl;
    assert(false);
    return RuntimeVal::Null();
}

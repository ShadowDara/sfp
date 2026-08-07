#include "statements.hpp"

using namespace fling;
using namespace fling::runtime;


// Function to evaluate a Program
fling::runtime::RuntimeVal fling::runtime::eval::evaluate_program(
    const fling::ast::Program &program,
    std::shared_ptr<runtime::envirment::Environment> env)
{
    // Store the last evaluated value, null as Default
    runtime::RuntimeVal last = runtime::RuntimeVal::Null();
    // loop through all statements in the program body
    for (const auto &stmt : program.body)
    {
        // STMT Null Check
        assert(stmt != nullptr);

        // STMT Deferenzieren to convert it to stmt&
        last = evaluate(*stmt, env);
    }

    // Return the last evaluated value
    return last;
}


// Function to evaluate a Variable Declaration
runtime::RuntimeVal fling::runtime::eval::evaluate_var_declaration(
    const ast::VarDeclaration &varDecl,
    std::shared_ptr<runtime::envirment::Environment> env)
{
    // Use a Reference instead of a smart pointer
    auto value = varDecl.value ? evaluate(*varDecl.value, env) : runtime::RuntimeVal();
    return env->declareVar(varDecl.identifier, std::move(value), varDecl.constant);
}


// Function to evalua a Function Declaration
runtime::RuntimeVal fling::runtime::eval::evaluate_fn_declaration(
    const ast::FunctionDeclaration& fnDecl,
    std::shared_ptr<runtime::envirment::Environment> env)
{
    std::vector<std::unique_ptr<ast::Stmt>> body;
    body.reserve(fnDecl.body.size());

    for (const auto& stmt : fnDecl.body)
    {
        if (stmt)
        {
            body.push_back(stmt->clone());
        }
    }

    RuntimeVal fn = RuntimeVal::Function(
        fnDecl.name,
        fnDecl.parameters,
        env->shared_from_this(),
        std::move(body)
    );

    return env->declareVar(fnDecl.name, std::move(fn), true);
}

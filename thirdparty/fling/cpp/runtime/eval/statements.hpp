#pragma once

#include <memory>

#include "../interpreter.hpp"
#include "../values.hpp"
#include "../envirments.hpp"
#include "../../frontend/ast.hpp"
#include "../../util.hpp"


namespace fling::runtime::eval {
    // Function to evaluate a Program Node
	fling::runtime::RuntimeVal evaluate_program(
		const ast::Program& program,
		std::shared_ptr<runtime::envirment::Environment> env);

    // Function to evaluate a Variable Declaration
	fling::runtime::RuntimeVal evaluate_var_declaration(
		const ast::VarDeclaration& varDecl,
		std::shared_ptr<runtime::envirment::Environment> env);

	// Function to evalua a Function Declaration
	fling::runtime::RuntimeVal evaluate_fn_declaration(
		const ast::FunctionDeclaration& fnDecl,
		std::shared_ptr<runtime::envirment::Environment> env);
}

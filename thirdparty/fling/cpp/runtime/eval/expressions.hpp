#pragma once

#include <memory>

#include "../interpreter.hpp"
#include "../values.hpp"
#include "../envirments.hpp"
#include "../../frontend/ast.hpp"
#include "../../util.hpp"


namespace fling::runtime::eval {
    fling::runtime::RuntimeVal evaluate_numeric_binary_expr(
		const fling::runtime::RuntimeVal &lhs,
		const fling::runtime::RuntimeVal &rhs,
		std::string callculation_operator,
		std::shared_ptr<runtime::envirment::Environment> env
	);

    // Function to evaluate a Binary Expression
		fling::runtime::RuntimeVal evaluate_binary_expr(
			const ast::BinaryExpr& binop,
			std::shared_ptr<runtime::envirment::Environment> env);

		// Function to evaluate an Identifier
		fling::runtime::RuntimeVal evaluate_identifier(
			const ast::Identifier& ident,
			std::shared_ptr<runtime::envirment::Environment> env);

		// Function to evaluate an Assignment Expression
		fling::runtime::RuntimeVal evaluate_assignment_expr(
			const ast::AssignmentExpr& node,
			std::shared_ptr<runtime::envirment::Environment> env);

		// Function to evaluate an Object Literal
		fling::runtime::RuntimeVal evaluate_object_expr(
			const ast::ObjectLiteral& node,
			std::shared_ptr<runtime::envirment::Environment> env);
		
		// Function to evaluate a Call Expression
		fling::runtime::RuntimeVal evaluate_call_expr(
            const ast::CallExpr& expr,
			std::shared_ptr<runtime::envirment::Environment> env);
}

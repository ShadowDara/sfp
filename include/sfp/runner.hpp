#pragma once


#include <unordered_set>
#include <string>
#include <stdexcept>
#include <string_view>
#include <iostream>
#include <variant>
#include <type_traits>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "colors.hpp"
#include "config.hpp"
#include "ast.hpp"
#include "generated.hpp"
#include "parser.hpp"
#include "preprocessor.hpp"


// function to run a task by name, with a set of visited tasks to avoid cycles
int run_task(
	const Tasks& tasks,
	std::string_view name,
	std::unordered_set<std::string>& visited,
	RuntimeState& state,
	const Config& conf);


// Function to run a samfile by name, with a set of visited tasks to avoid cycles
int run_samfile(
	std::string_view name,
	const Config& conf);

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
#include <vector>

#include "colors.hpp"
#include "config.hpp"
#include "ast.hpp"
#include "generated.hpp"
#include "parser.hpp"
#include "preprocessor.hpp"

#ifdef _WIN32
	#include <windows.h>
#else
	#include <unistd.h>
	#include <sys/wait.h>
#endif


// Function to run a command
void runCommand(
	const std::string& command,
	const RuntimeState& state
);


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

#pragma once

#include <string>
#include <unordered_map>
#include <sstream>
#include <unordered_set>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <algorithm>

#include "parser.hpp"


enum class VisitState
{
    NotVisited,
    Visiting,
    Visited
};


std::string replace_macros(
	std::string_view input,
	const std::unordered_map<std::string, std::string>& defines);


std::string remove_define_lines(const std::string& input);


std::string preprocess(const std::string& input);


// Function to detect cycles in the task dependencies
void validate_all(const Tasks& tasks);


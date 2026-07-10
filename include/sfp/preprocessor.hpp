#pragma once

#include <string>
#include <unordered_map>
#include <sstream>

#include "parser.hpp"


std::string replace_macros(
	std::string_view input,
	const std::unordered_map<std::string, std::string>& defines);


std::string remove_define_lines(const std::string& input);


std::string proprocess(const std::string& input);

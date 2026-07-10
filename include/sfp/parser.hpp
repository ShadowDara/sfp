#pragma once

#include <string>
#include <cctype>
#include <optional>
#include <vector>
#include <sstream>
#include <unordered_map>
#include <iostream>
#include <string_view>


#include "ast.hpp"
#include "config.hpp"
#include "preprocessor.hpp"
#include "string_utils.hpp"


CommandType parse_command(std::string_view s);


// Function to parse a single line of input into a Command object
std::optional<Command> parse_line(const std::string& line, const Config& config, std::size_t idx);


// Function to parse the Task Header
TaskHeader parse_task_header(const std::string& line);


// Function to parse the complete file
Tasks parse(std::string content, const Config& conf);

#pragma once

#include <optional>

#include "ast.hpp"
#include "config.hpp"


// Function to parse a single line of input into a Command object
std::optional<Command> parse_line(const std::string& line, const Config& config, std::size_t idx);


// Function to parse the Task Header
TaskHeader parse_task_header(const std::string& line);


// Function to parse the complete file
Tasks parse(const std::string& input, const Config& config);

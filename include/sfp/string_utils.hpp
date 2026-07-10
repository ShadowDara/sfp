#pragma once


#include <string>
#include <vector>
#include <string_view>
#include <cctype>
#include <sstream>


// trim spaces
std::string trim(const std::string& s);


// Split by input by whitespace
std::vector<std::string> split_whitespace(const std::string& input);


std::string_view trim_view(std::string_view s);

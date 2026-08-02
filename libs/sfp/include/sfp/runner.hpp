#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_set>
#include <variant>
#include <vector>

#include "ast.hpp"
#include "colors.hpp"
#include "config.hpp"
#include "generated.hpp"
#include "parser.hpp"
#include "preprocessor.hpp"

#ifdef _WIN32
    #define NOMINMAX
    #include <windows.h>
#else
    #include <cstring>
    #include <sys/wait.h>
    #include <unistd.h>
#endif

// Function to run a command
void runCommand(const std::string &command, const RuntimeState &state);

// function to run a task by name, with a set of visited tasks to avoid cycles
int run_task(const Tasks &tasks, std::string_view name,
             std::unordered_set<std::string> &visited, RuntimeState &state,
             const Config &conf);

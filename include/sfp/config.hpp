#pragma once

#include <string>
#include <vector>
#include <unordered_map>


struct Config {};

struct TaskHeader {
	std::string name;
	std::vector<std::string> dependencies;
};

struct CommandWithMeta {
	Command command;
	std::size_t line_number;
	std::string original_line;
};

struct Task {
	std::vector<std::string> dependencies;
	std::vector<CommandWithMeta> commands;
};

using Tasks = std::unordered_map<std::string, Task>;

#pragma once


#include <unordered_set>
#include <string>
#include <stdexcept>

#include "config.hpp"


int run_task(
	Tasks tasks,
	std::string name,
	std::unordered_set<std::string> visited,
	RuntimeState state,
	Config conf);


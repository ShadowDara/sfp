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


// Function to detect cycles in the task dependencies
void validate_all(const Tasks& tasks);


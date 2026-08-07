// Utils Header
// Contains some Util Functions for the Language
// Probably Temporary

#pragma once

#include <memory>
#include <iostream>


// Error Macro
#define ERROR(msg) \
    std::cerr << std::format("[{}:{}] {}(): {}\n", \
        __FILE__, __LINE__, __func__, msg)


namespace fling::util
{
	// Convert a float to an Integer
	int toInt(float number);
}

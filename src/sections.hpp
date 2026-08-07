#pragma once

// Section Parser
//


#define SECTION_KEYWORD "%%section "
#define SECTION_END_KEYWORD "%%endsection"

#include <string>
#include <unordered_map>
#include <strutil.hpp>
#include <vector>


// Function to parse the Data
std::unordered_map<std::string, std::string> parse_Sections(const std::string &input)
{
	std::unordered_map<std::string, std::string> out;

	// Split by lines
	std::vector<std::string> lines = strutil::splitlines(input); 

	std::string active_sec = "";

	for (auto const &e : lines)
	{
		if (strutil::starts_with(e, SECTION_KEYWORD))
		{
			// Get the name of the new section
			std::string name = strutil::remove_prefix(e, SECTION_KEYWORD);
			
			// Set it as the active section
			active_sec = name;
			
			continue;
		}

		if (strutil::starts_with(e, SECTION_END_KEYWORD))
		{
			active_sec = "";
			continue;
		}
		
		// add the line the Map
		out[active_sec] += e;
	}

	return out;
}


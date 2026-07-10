#include <sfp/preprocessor.hpp>


// function to preprocess the macros in the input string
std::string proprocess(const std::string& input)
{
	std::string output = input;

	std::unordered_map<std::string, std::string> defines;

	std::istringstream stream(input);
	std::string line;

	// Iterate over the lines of the input and process the macros
	while (std::getline(stream, line))
	{
		line = trim(line);
		
		if (line.starts_with("#define "))
		{
			
		}
	}

	return output;
}

#include <sfp/preprocessor.hpp>


std::string replace_macros(
	std::string_view input,
	const std::unordered_map<std::string, std::string>& defines)
{
	std::string output;

	size_t i = 0;

	while (i < input.size())
	{
		// Identifier Anfang?
		if (std::isalpha(static_cast<unsigned char>(input[i])) ||
			input[i] == '_')
		{
			size_t start = i;

			while (i < input.size() &&
				(std::isalnum(static_cast<unsigned char>(input[i])) ||
					input[i] == '_'))
			{
				i++;
			}

			std::string_view word = input.substr(start, i - start);

			auto it = defines.find(std::string(word));

			if (it != defines.end())
				output += it->second;
			else
				output.append(word);
		}
		else
		{
			output += input[i];
			i++;
		}
	}

	return output;
}


std::string remove_define_lines(const std::string& input)
{
	std::stringstream in(input);
	std::string output;
	std::string line;

	while (std::getline(in, line))
	{
		std::string_view trimmed = line;

		// führende Whitespaces überspringen
		while (!trimmed.empty() && std::isspace(trimmed.front()))
			trimmed.remove_prefix(1);

		if (trimmed.starts_with("#define "))
			continue;

		output += line;
		output += '\n';
	}

	return output;
}


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
			auto parts = split_whitespace(line);

			// We need 3 parts
			if (parts.size() >= 3)
			{
				// Add the parts
				defines.insert({ parts[1], parts[2] });
			}
		}
	}

	// Remove Macro lines
	output = remove_define_lines(output);

	// Replace in the Macro Usage
	output = replace_macros(output, defines);

	return output;
}

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
std::string preprocess(const std::string& input)
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


// Function to detect cycles in the task dependencies
void detect_cycles(
    const Tasks& tasks,
    std::string_view name,
    std::unordered_map<std::string, VisitState>& state,
    std::vector<std::string>& stack
)
{
    auto it = state.find(std::string(name));

    VisitState current = VisitState::NotVisited;

    if (it != state.end())
        current = it->second;


    switch (current)
    {
    case VisitState::Visiting:
    {
        // cycle found

        auto cycle_start = std::find(
            stack.begin(),
            stack.end(),
            name
        );

        if (cycle_start == stack.end())
        {
            throw std::runtime_error(
                "Internal error: cycle detection state corrupted"
            );
        }


        std::vector<std::string> cycle(
            cycle_start,
            stack.end()
        );


        std::string msg = "Cycle detected: ";

        for (auto& n : cycle)
        {
            msg += n;
            msg += " -> ";
        }

        throw std::runtime_error(msg);
    }


    case VisitState::Visited:
        return;


    case VisitState::NotVisited:
        break;
    }


    // mark as visiting
    state[std::string(name)] = VisitState::Visiting;

    stack.emplace_back(name);


    auto task_it = tasks.find(std::string(name));

    if (task_it == tasks.end())
    {
        throw std::runtime_error(
            "task not found: " + std::string(name)
        );
    }


    const Task& task = task_it->second;


    // Check unknown dependencies
    for (const auto& dep : task.dependencies)
    {
        if (!tasks.contains(dep))
        {
            throw std::runtime_error(
                "Unknown dependency '"
                + dep
                + "' in task '"
                + std::string(name)
                + "'"
            );
        }
    }


    // recurse
    for (const auto& dep : task.dependencies)
    {
        detect_cycles(
            tasks,
            dep,
            state,
            stack
        );
    }


    stack.pop_back();

    state[std::string(name)] = VisitState::Visited;
}


// Function to validate all tasks for cycles
void validate_all(const Tasks& tasks)
{
    std::unordered_map<std::string, VisitState> state;

    std::vector<std::string> stack;


    for (const auto& [name, task] : tasks)
    {
        detect_cycles(
            tasks,
            name,
            state,
            stack
        );
    }
}

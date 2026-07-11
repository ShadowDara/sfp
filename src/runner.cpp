#include <sfp/runner.hpp>


int run_task(
	const Tasks& tasks,
	std::string_view name,
	std::unordered_set<std::string>& visited,
	RuntimeState& state,
	const Config& conf)
{
	// Check if the Task was already visited (executed)
	if (visited.contains(std::string(name)))
	{
		return 1;
	}

	// Serach Task
	auto it = tasks.find(std::string(name));

	if (it == tasks.end())
	{
		// TASK NOT FOUND
		
		std::cout << RED << "Error" << END << " : Task '" << name << "' not found\n"
			<< "Available tasks are:\n";

		// Display als available tasks
		for (const auto& [key, value] : tasks)
		{
			std::cout << "  - " << key << '\n';
		}

		return 1;
	}

	const Task& task = it->second;


	// add to visited liste
	visited.insert(std::string(name));


	// Create a local copy of the RuntimeState for this task
	RuntimeState local_state{
		.path = state.path,
		.env_vars = state.env_vars
	};


	// Run dependencies first
	for (const auto& dep : task.dependencies)
	{
		run_task(
			tasks,
			dep,
			visited,
			local_state,
			conf
		);
	}


	std::cout << "==> " << GREEN << "Running task" << END << " : " << name << '\n';

	for (const auto& cmd_with_meta : task.commands)
	{
		// Print Info about the command being executed
		std::cout << GREEN << "[EXECUTING]" << END << " "
			<< cmd_with_meta.original_line << '\n';

		std::visit([&](const auto& cmd)
			{
				using T = std::decay_t<decltype(cmd)>;

				// CD
				if constexpr (std::is_same_v<T, Cd>)
				{
					// cmd.path
				}

				// RUN
				else if constexpr (std::is_same_v<T, Run>)
				{
					// cmd.program
				}

				// ENV
				else if constexpr (std::is_same_v<T, Env>)
				{
					// cmd.key
					// cmd.value
				}

				// SLEEP
				else if constexpr (std::is_same_v<T, Sleep>)
				{
					// ...
				}

				// ERROR
				else
				{
					// weitere Typen...
				}

			}, cmd_with_meta.command);
	}

	// Continue

	return 0;
}


// Function to run a samfile by name, with a set of visited tasks to avoid cycles
int run_samfile(std::string_view command, const Config& conf)
{
	RuntimeState state{
		.path = std::filesystem::current_path().string(),
		.env_vars = {}        
	};

	std::string content2 = "";

	std::ifstream file(".samengine/samfile");

	if (!file)
	{
		std::cerr << RED <<"Error while reading samfile" << END << "\n";
		
		// Do not exit here, because users should still be able
		// to run buildin tasks with out a samfile
		//return 1;

		// Set the buildin samfile into the content
		content2 = buildin_samfile_content;
	}

	// When file was able to read
	else
	{
		std::stringstream buffer;
		buffer << file.rdbuf();

		std::string content = buffer.str();

		// combine built-in + file
		content2 =
			buildin_samfile_content +
			"\n\n" +
			content;
	}

	Tasks tasks = parse(content2, conf);

	// Check for cycled dependencies
	validate_all(tasks);

	// already visited tasks
	std::unordered_set<std::string> visited;

	// Execute task
	run_task(
		tasks,
		command,
		visited,
		state,
		conf
	);

	return 0;
}

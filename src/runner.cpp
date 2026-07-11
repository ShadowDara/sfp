#include <sfp/runner.hpp>

// Filesystem
namespace fs = std::filesystem;


#ifdef _WIN32
	// Function to run a command
	void runCommand(
		const std::string& command,
		const RuntimeState& state
	)
	{
		std::stringstream ss(command);

		std::string program;
		ss >> program;

		std::string args;
		std::getline(ss, args);


		std::string cmdLine = program + args;


		STARTUPINFOA si{};
		PROCESS_INFORMATION pi{};

		si.cb = sizeof(si);


		BOOL result = CreateProcessA(
			nullptr,
			cmdLine.data(),              // command line
			nullptr,
			nullptr,
			FALSE,
			0,
			nullptr,                     // environment
			state.path.string().c_str(),  // working directory
			&si,
			&pi
		);


		if (!result)
		{
			std::cerr
				<< "failed to start process: "
				<< GetLastError()
				<< '\n';

			return;
		}


		WaitForSingleObject(
			pi.hProcess,
			INFINITE
		);


		DWORD exitCode;

		GetExitCodeProcess(
			pi.hProcess,
			&exitCode
		);


		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);


		if (exitCode != 0)
		{
			std::cerr << "task failed\n";
		}
	}

#else
	// Version Linux, MacOS
	int runCommand(
		const std::string& program,
		const std::vector<std::string>& args,
		const fs::path& cwd
	)
	{
		pid_t pid = fork();


		if (pid == -1)
		{
			std::cerr << "fork failed\n";
			return -1;
		}


		// Child process
		if (pid == 0)
		{
			// Working directory setzen
			if (chdir(cwd.c_str()) != 0)
			{
				std::cerr << "chdir failed\n";
				exit(1);
			}


			// argv für exec bauen
			std::vector<char*> argv;

			argv.push_back(
				const_cast<char*>(program.c_str())
			);


			for (auto& arg : args)
			{
				argv.push_back(
					const_cast<char*>(arg.c_str())
				);
			}


			argv.push_back(nullptr);


			execvp(
				program.c_str(),
				argv.data()
			);


			// Nur erreicht wenn exec fehlschlägt
			std::cerr << "exec failed\n";
			exit(1);
		}


		// Parent process wartet
		int status;

		waitpid(
			pid,
			&status,
			0
		);


		if (WIFEXITED(status))
		{
			return WEXITSTATUS(status);
		}


		return -1;
	}
#endif


// Function to run a samfile task
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
					std::error_code ec;

					fs::path p(cmd.path);

					fs::path canonical = fs::canonical(p, ec);

					if (ec)
					{
						std::cerr << "cd failed: "
							<< ec.message() << '\n';
						//return 0;
					}

					state.path = canonical;
				}

				// RUN
				else if constexpr (std::is_same_v<T, Run>)
				{
					runCommand(cmd.name, state);
				}

				// ENV
				else if constexpr (std::is_same_v<T, Env>)
				{
					// cmd.key
					// cmd.value
				}

				// SLEEP
				else if constexpr (std::is_same_v<T, SleepTask>)
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

	std::ifstream file("samfile");

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

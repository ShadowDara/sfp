#include <sfp/runner.hpp>

// Filesystem
namespace fs = std::filesystem;

#ifdef _WIN32
// Function to run a command
void runCommand(const std::string &command, const RuntimeState &state)
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

    BOOL result =
        CreateProcessA(nullptr,
                       cmdLine.data(), // command line
                       nullptr, nullptr, FALSE, 0,
                       nullptr,                     // environment
                       state.path.string().c_str(), // working directory
                       &si, &pi);

    if (!result)
    {
        std::cerr << "failed to start process: " << GetLastError() << '\n';

        return;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode;

    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    if (exitCode != 0)
    {
        std::cerr << "task failed\n";
    }
}

#else

void runCommand(const std::string &command, const RuntimeState &state)
{
    std::stringstream ss(command);

    std::string program;
    ss >> program;

    if (program.empty())
    {
        std::cerr << "empty command\n";
        return;
    }

    std::vector<std::string> args;

    std::string arg;
    while (ss >> arg)
    {
        args.push_back(arg);
    }

    pid_t pid = fork();

    if (pid == -1)
    {
        std::cerr << "fork failed\n";
        return;
    }

    // Child process
    if (pid == 0)
    {
        // Arbeitsverzeichnis setzen
        if (chdir(state.path.c_str()) != 0)
        {
            std::cerr << "chdir failed: " << strerror(errno) << '\n';
            exit(1);
        }

        // argv für execvp bauen
        std::vector<char *> argv;

        argv.push_back(const_cast<char *>(program.c_str()));

        for (auto &a : args)
        {
            argv.push_back(const_cast<char *>(a.c_str()));
        }

        argv.push_back(nullptr);

        execvp(program.c_str(), argv.data());

        // Wird nur erreicht wenn execvp fehlschlägt
        std::cerr << "exec failed: " << strerror(errno) << '\n';

        exit(1);
    }

    // Parent wartet auf Prozess
    int status = 0;

    waitpid(pid, &status, 0);

    if (WIFEXITED(status))
    {
        int exitCode = WEXITSTATUS(status);

        if (exitCode != 0)
        {
            std::cerr << "task failed with exit code " << exitCode << '\n';
        }
    }
    else
    {
        std::cerr << "task terminated unexpectedly\n";
    }
}

#endif

// Function to run a samfile task
int run_task(const Tasks &tasks, std::string_view name,
             std::unordered_set<std::string> &visited, RuntimeState &state,
             const Config &conf)
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

        std::cout << RED << "Error" << END << " : Task '" << name
                  << "' not found\n"
                  << "Available tasks are:\n";

        // Display als available tasks
        for (const auto &[key, value] : tasks)
        {
            std::cout << "  - " << key << '\n';
        }

        return 1;
    }

    const Task &task = it->second;

    // add to visited liste
    visited.insert(std::string(name));

    // Create a local copy of the RuntimeState for this task
    RuntimeState local_state{.path = state.path, .env_vars = state.env_vars};

    // Run dependencies first
    for (const auto &dep : task.dependencies)
    {
        run_task(tasks, dep, visited, local_state, conf);
    }

    std::cout << "==> " << GREEN << "Running task" << END << " : " << name
              << '\n';

    for (const auto &cmd_with_meta : task.commands)
    {
        // Print Info about the command being executed
        std::cout << GREEN << "[EXECUTING]" << END << " "
                  << cmd_with_meta.original_line << '\n';

        std::visit(
            [&](const auto &cmd)
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
                        std::cerr << "cd failed: " << ec.message() << '\n';
                        // return 0;
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
            },
            cmd_with_meta.command);
    }

    // Continue

    return 0;
}

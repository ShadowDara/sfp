#include <sfp/parser.hpp>

CommandType parse_command(std::string_view s)
{
    if (s == "CD")
    {
        return CommandType::Cd;
    }
    if (s == "ENV")
    {
        return CommandType::Env;
    }
    if (s == "RUN")
    {
        return CommandType::Run;
    }
    if (s == "task")
    {
        return CommandType::ExecuteTask;
    }
    if (s == "rm")
    {
        return CommandType::Rm;
    }
    if (s == "mkdir")
    {
        return CommandType::Mkdir;
    }
    if (s == "cp")
    {
        return CommandType::Cp;
    }
    if (s == "mv")
    {
        return CommandType::Mv;
    }
    if (s == "sleep")
    {
        return CommandType::Sleep;
    }
    if (s == "shell")
    {
        return CommandType::Shell;
    }
    if (s == "echo")
    {
        return CommandType::Echo;
    }
    if (s == "warn")
    {
        return CommandType::Warn;
    }
    if (s == "error")
    {
        return CommandType::Error;
    }
    if (s == "touch")
    {
        return CommandType::Touch;
    }
    if (s == "write")
    {
        return CommandType::Write;
    }
    if (s == "append")
    {
        return CommandType::Append;
    }
    if (s == "unsetenv")
    {
        return CommandType::UnsetEnv;
    }
    if (s == "prompt")
    {
        return CommandType::Prompt;
    }
    if (s == "win")
    {
        return CommandType::Win;
    }
    if (s == "lin")
    {
        return CommandType::Lin;
    }
    if (s == "mac")
    {
        return CommandType::Mac;
    }

    return CommandType::ErrorType;
}

// Function to parse a single line of input into a Command object
std::optional<Command> parse_line(const std::string &line, const Config &config,
                                  std::size_t idx)
{
    auto args = split_whitespace(line);

    CommandWithMeta metadata = {.command = ErrorType{CommandType::ErrorType},
                                .line_number = idx,
                                .original_line = line};

    if (args.empty())
    {
        return std::nullopt;
    }

    switch (parse_command(args[0]))
    {
    // CD
    case CommandType::Cd:
    {
        if (args.size() != 2)
        {
            throw std::runtime_error("Invalid CD command at line " +
                                     std::to_string(idx));
        }
        return Cd{CommandType::Cd, args[1]};
    }

    case CommandType::CdWin:
    {
        if (args.size() != 2)
        {
            throw std::runtime_error("Invalid CD command at line " +
                                     std::to_string(idx));
        }
        return Cd{CommandType::CdWin, args[1]};
    }

    case CommandType::CdMac:
    {
        if (args.size() != 2)
        {
            throw std::runtime_error("Invalid CD command at line " +
                                     std::to_string(idx));
        }
        return Cd{CommandType::CdMac, args[1]};
    }

    case CommandType::CdLin:
    {
        if (args.size() != 2)
        {
            throw std::runtime_error("Invalid CD command at line " +
                                     std::to_string(idx));
        }
        return Cd{CommandType::CdLin, args[1]};
    }

    // RUN
    case CommandType::Run:
    {
        std::string command;

        for (size_t i = 1; i < args.size(); i++)
        {
            command += args[i];
            command += " ";
        }

        return Run{CommandType::Run, command};
    }

    case CommandType::RunWin:
    {
        std::string command;

        for (size_t i = 1; i < args.size(); i++)
        {
            command += args[i];
            command += " ";
        }

        return Run{CommandType::RunWin, command};
    }

    case CommandType::RunMac:
    {
        std::string command;

        for (size_t i = 1; i < args.size(); i++)
        {
            command += args[i];
            command += " ";
        }

        return Run{CommandType::RunMac, command};
    }

    case CommandType::RunLin:
    {
        std::string command;

        for (size_t i = 1; i < args.size(); i++)
        {
            command += args[i];
            command += " ";
        }

        return Run{CommandType::RunLin, command};
    }

    // ENV
    case CommandType::Env:
    {
        const std::string &arg = args[1];

        auto pos = arg.find('=');

        if (pos != std::string::npos)
        {
            std::string key = arg.substr(0, pos);
            std::string value = arg.substr(pos + 1);

            trim(key);

            return Env{CommandType::Env, key, value};
        }

        handle_failure("Invalid env command: env KEY=VALUE", config, metadata);
        return std::nullopt;
    }

    case CommandType::EnvWin:
    {
        const std::string &arg = args[1];

        auto pos = arg.find('=');

        if (pos != std::string::npos)
        {
            std::string key = arg.substr(0, pos);
            std::string value = arg.substr(pos + 1);

            trim(key);

            return Env{CommandType::EnvWin, key, value};
        }

        handle_failure("Invalid env command: env KEY=VALUE", config, metadata);
        return std::nullopt;
    }

    case CommandType::EnvMac:
    {
        const std::string &arg = args[1];

        auto pos = arg.find('=');

        if (pos != std::string::npos)
        {
            std::string key = arg.substr(0, pos);
            std::string value = arg.substr(pos + 1);

            trim(key);

            return Env{CommandType::EnvMac, key, value};
        }

        handle_failure("Invalid env command: env KEY=VALUE", config, metadata);
        return std::nullopt;
    }

    case CommandType::EnvLin:
    {
        const std::string &arg = args[1];

        auto pos = arg.find('=');

        if (pos != std::string::npos)
        {
            std::string key = arg.substr(0, pos);
            std::string value = arg.substr(pos + 1);

            trim(key);

            return Env{CommandType::EnvLin, key, value};
        }

        handle_failure("Invalid env command: env KEY=VALUE", config, metadata);
        return std::nullopt;
    }

        // Others add here
    case CommandType::ExecuteTask:
    case CommandType::ExecuteTaskWin:
    case CommandType::ExecuteTaskMac:
    case CommandType::ExecuteTaskLin:
    case CommandType::Rm:
    case CommandType::RmWin:
    case CommandType::RmMac:
    case CommandType::RmLin:
    case CommandType::Mkdir:
    case CommandType::MkdirWin:
    case CommandType::MkdirMac:
    case CommandType::MkdirLin:
    case CommandType::Cp:
    case CommandType::CpWin:
    case CommandType::CpMac:
    case CommandType::CpLin:
    case CommandType::Mv:
    case CommandType::MvWin:
    case CommandType::MvMac:
    case CommandType::MvLin:
    case CommandType::Sleep:
    case CommandType::SleepWin:
    case CommandType::SleepMac:
    case CommandType::SleepLin:
    case CommandType::Shell:
    case CommandType::ShellWin:
    case CommandType::ShellMac:
    case CommandType::ShellLin:
    case CommandType::Echo:
    case CommandType::EchoWin:
    case CommandType::EchoMac:
    case CommandType::EchoLin:
    case CommandType::Warn:
    case CommandType::WarnWin:
    case CommandType::WarnMac:
    case CommandType::WarnLin:
    case CommandType::Error:
    case CommandType::ErrorWin:
    case CommandType::ErrorMac:
    case CommandType::ErrorLin:
    case CommandType::Touch:
    case CommandType::TouchWin:
    case CommandType::TouchMac:
    case CommandType::TouchLin:
    case CommandType::Write:
    case CommandType::WriteWin:
    case CommandType::WriteMac:
    case CommandType::WriteLin:
    case CommandType::Append:
    case CommandType::AppendWin:
    case CommandType::AppendMac:
    case CommandType::AppendLin:
    case CommandType::UnsetEnv:
    case CommandType::UnsetEnvWin:
    case CommandType::UnsetEnvMac:
    case CommandType::UnsetEnvLin:
    case CommandType::Prompt:
    case CommandType::PromptWin:
    case CommandType::PromptMac:
    case CommandType::PromptLin:
    case CommandType::Win:
    case CommandType::Lin:
    case CommandType::Mac:
    case CommandType::ErrorType:
        break;
    }

    return std::nullopt;
}

// Function to parse the Task Header
TaskHeader parse_task_header(const std::string &line)
{
    // Search for the colon in the line
    size_t colon_pos = line.find(':');
    if (colon_pos == std::string::npos)
    {
        throw std::runtime_error("Invalid task header: missing ':' in line: " +
                                 line);
    }

    // Extract the task name and dependencies
    std::string name = trim(line.substr(0, colon_pos));
    std::string deps_str = trim(line.substr(colon_pos + 1));

    // Split dependencies by whitespace
    std::vector<std::string> dependencies = split_whitespace(deps_str);
    return TaskHeader{name, dependencies};
}

// Function to parse the complete file
Tasks parse(std::string content, const Config &conf)
{
    Tasks tasks;
    std::optional<std::string> current;

    std::istringstream stream{std::string(content)};
    std::string line;

    size_t idx = 0;

    while (std::getline(stream, line))
    {
        idx++;
        size_t line_no = idx;

        // trim_end()
        while (!line.empty() &&
               std::isspace(static_cast<unsigned char>(line.back())))
        {
            line.pop_back();
        }

        auto trimmed = trim_view(line);

        // ignore empty lines
        if (trimmed.empty())
            continue;

        // ignore comments
        if (trimmed.starts_with('#') || trimmed.starts_with("//") ||
            trimmed.starts_with("--"))
        {
            continue;
        }

        // task header
        //
        // Search for a colon in the line and check
        // if the first character is not whitespace
        if (!line.empty() &&
            !std::isspace(static_cast<unsigned char>(line[0])) &&
            line.find(':') != std::string::npos)
        {
            auto [name, deps] = parse_task_header(line);

            if (tasks.contains(name))
            {
                std::cout << "Error: Duplicate task '" << name << "' at line "
                          << line_no << std::endl;
                throw std::runtime_error("Duplicate task '" + name + "'");
            }

            tasks.emplace(name, Task{.dependencies = deps, .commands = {}});

            current = name;
        }

        // command
        else if (!line.empty() &&
                 std::isspace(static_cast<unsigned char>(line[0])))
        {
            if (current.has_value())
            {
                auto cmd = parse_line(line, conf, idx);

                if (cmd.has_value())
                {
                    auto &task = tasks.at(*current);

                    task.commands.push_back(
                        CommandWithMeta{.command = *cmd,
                                        .line_number = line_no,
                                        .original_line = line});
                }
                else
                {
                    std::cerr << "warning: ignored invalid line: " << line
                              << "\n";
                }
            }
        }

        else
        {
            std::cerr << "warning: line outside of task: " << line << "\n";
        }
    }

    return tasks;
}

void handle_failure(std::string msg, Config conf, CommandWithMeta cmd) {}

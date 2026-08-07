#pragma once

#include <string>
#include <variant>

enum class CommandType
{
    Cd,
    CdWin,
    CdMac,
    CdLin,

    Env,
    EnvWin,
    EnvMac,
    EnvLin,

    Run,
    RunWin,
    RunMac,
    RunLin,

    ExecuteTask,
    ExecuteTaskWin,
    ExecuteTaskMac,
    ExecuteTaskLin,

    Rm,
    RmWin,
    RmMac,
    RmLin,

    Mkdir,
    MkdirWin,
    MkdirMac,
    MkdirLin,

    Cp,
    CpWin,
    CpMac,
    CpLin,

    Mv,
    MvWin,
    MvMac,
    MvLin,

    Sleep,
    SleepWin,
    SleepMac,
    SleepLin,

    Shell,
    ShellWin,
    ShellMac,
    ShellLin,

    Echo,
    EchoWin,
    EchoMac,
    EchoLin,

    Warn,
    WarnWin,
    WarnMac,
    WarnLin,

    Error,
    ErrorWin,
    ErrorMac,
    ErrorLin,

    Touch,
    TouchWin,
    TouchMac,
    TouchLin,

	Write,
    WriteWin,
    WriteMac,
    WriteLin,

	Append,
    AppendWin,
    AppendMac,
    AppendLin,

	UnsetEnv,
    UnsetEnvWin,
    UnsetEnvMac,
    UnsetEnvLin,

	Prompt,
    PromptWin,
    PromptMac,
    PromptLin,

	Win,
	Lin,
	Mac,

	ErrorType
};


struct Cd {
    CommandType type;
	std::string path;
};

struct Env {
    CommandType type;
	std::string name;
	std::string value;
};

struct Run {
    CommandType type;
	std::string name;
};

struct ExecuteTask {
    CommandType type;
	std::string name;
};

struct Rm {
    CommandType type;
	std::string name;
};

struct Mkdir {
    CommandType type;
	std::string name;
};

struct Cp {
    CommandType type;
	std::string source;
	std::string destination;
};

struct Mv {
    CommandType type;
	std::string source;
	std::string destination;
};

struct SleepTask {
    CommandType type;
	int seconds;
};

struct Shell {
    CommandType type;
	std::string command;
};

struct Echo {
    CommandType type;
	std::string message;
};

struct Warn {
    CommandType type;
	std::string message;
};

struct Error {
    CommandType type;
	std::string message;
};

struct Touch {
    CommandType type;
	std::string name;
};

struct Write {
    CommandType type;
	std::string name;
	std::string content;
};

struct Append {
    CommandType type;
	std::string name;
	std::string content;
};

struct UnsetEnv {
    CommandType type;
	std::string name;
};

struct Prompt {
    CommandType type;
	std::string message;
};

// for ErrorType, we can use a variant to represent different error types
struct ErrorType {
    CommandType type;
};


using Command = std::variant<
	Cd,
	Env,
	Run,
	ExecuteTask,
	Rm,
	Mkdir,
	Cp,
	Mv,
	SleepTask,
	Shell,
	Echo,
	Warn,
	Error,
	Touch,
	Write,
	Append,
	UnsetEnv,
	Prompt,
	ErrorType
>;

#pragma once

#include <string>
#include <variant>

enum class CommandType
{
	Cd,
	Env,
	Run,
	ExecuteTask,
	Rm,
	Mkdir,
	Cp,
	Mv,
	Sleep,
	Shell,
	Echo,
	Warn,
	Error,
	Touch,
	Write,
	Append,
	UnsetEnv,
	Prompt,

	Win,
	Lin,
	Mac,

	ErrorType
};


struct Cd {
	std::string path;
};

struct Env {
	std::string name;
	std::string value;
};

struct Run {
	std::string name;
};

struct ExecuteTask {
	std::string name;
};

struct Rm {
	std::string name;
};

struct Mkdir {
	std::string name;
};

struct Cp {
	std::string source;
	std::string destination;
};

struct Mv {
	std::string source;
	std::string destination;
};

struct SleepTask {
	int seconds;
};

struct Shell {
	std::string command;
};

struct Echo {
	std::string message;
};

struct Warn {
	std::string message;
};

struct Error {
	std::string message;
};

struct Touch {
	std::string name;
};

struct Write {
	std::string name;
	std::string content;
};

struct Append {
	std::string name;
	std::string content;
};

struct UnsetEnv {
	std::string name;
};

struct Prompt {
	std::string message;
};

// for ErrorType, we can use a variant to represent different error types
struct ErrorType {

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

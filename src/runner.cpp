#include <sfp/runner.hpp>


int run_task(
	Tasks tasks,
	std::string name,
	std::unordered_set<std::string> visited,
	RuntimeState state,
	Config conf)
{
	if (visited.find(name) != visited.end())
	{
		throw std::runtime_error(
			"Circular dependency detected for task '" + name + "'"
		);
	}

	// Continue
}

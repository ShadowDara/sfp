#include <sfp/preprocessor.hpp>


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

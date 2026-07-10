#include <sfp/parser.hpp>


std::string trim(const std::string& s)
{
    size_t start = 0;
    size_t end = s.size();

    while (start < end && std::isspace(static_cast<unsigned char>(s[start])))
        start++;

    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1])))
        end--;

    return s.substr(start, end - start);
}


std::vector<std::string> split_whitespace(const std::string& input)
{
    std::vector<std::string> result;
    std::istringstream iss(input);

    std::string word;
    while (iss >> word) {
        result.push_back(word);
    }

    return result;
}


// Function to parse a single line of input into a Command object
std::optional<Command> parse_line(const std::string& line, const Config& config, std::size_t idx)
{
}


// Function to parse the Task Header
TaskHeader parse_task_header(const std::string& line)
{
}

// Function to parse the complete file
Tasks parse(const std::string& input, const Config& config)
{
	// add preprocessing step here if needed
    auto content = preprocess(input);

    
}


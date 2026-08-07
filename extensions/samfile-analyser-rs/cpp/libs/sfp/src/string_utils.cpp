#include <sfp/string_utils.hpp>


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


std::string_view trim_view(std::string_view s)
{
    while (!s.empty() &&
        std::isspace(static_cast<unsigned char>(s.front())))
    {
        s.remove_prefix(1);
    }

    while (!s.empty() &&
        std::isspace(static_cast<unsigned char>(s.back())))
    {
        s.remove_suffix(1);
    }

    return s;
}

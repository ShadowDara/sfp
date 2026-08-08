#pragma once

// Section Parser
//

#define SECTION_KEYWORD "%%section "
#define SECTION_END_KEYWORD "%%endsection"

#include <string>
#include <strutil.hpp>
#include <unordered_map>
#include <vector>

struct ParsedSection
{
    std::string name;
    size_t start_line;
    size_t end_line;
    std::string content;
};

// Function to parse the Data
inline std::unordered_map<std::string, std::string>
parse_Sections(const std::string &input)
{
    std::unordered_map<std::string, std::string> out;

    // Split by lines
    std::vector<std::string> lines = strutil::splitlines(input);

    std::string active_sec = "";

    for (auto const &e : lines)
    {
        if (strutil::starts_with(e, SECTION_KEYWORD))
        {
            // Get the name of the new section
            std::string name = strutil::remove_prefix(e, SECTION_KEYWORD);

            // Set it as the active section
            active_sec = name;

            continue;
        }

        if (strutil::starts_with(e, SECTION_END_KEYWORD))
        {
            active_sec = "";
            continue;
        }

        // add the line the Map
        out[active_sec] += e;
    }

    return out;
}

inline std::vector<ParsedSection>
parse_sections_with_ranges(const std::string &input)
{
    std::vector<ParsedSection> out;

    auto lines = strutil::splitlines(input);

    ParsedSection current{};
    bool in_section = false;

    for (size_t line_no = 0; line_no < lines.size(); ++line_no)
    {
        const auto &line = lines[line_no];

        if (strutil::starts_with(line, SECTION_KEYWORD))
        {
            if (in_section)
            {
                current.end_line = line_no - 1;
                out.push_back(current);
            }

            current = {};

            current.name = strutil::remove_prefix(line, SECTION_KEYWORD);

            current.start_line = line_no;

            in_section = true;

            continue;
        }

        if (strutil::starts_with(line, SECTION_END_KEYWORD))
        {
            if (in_section)
            {
                current.end_line = line_no;
                out.push_back(current);
            }

            current = {};
            in_section = false;

            continue;
        }

        if (in_section)
        {
            current.content += line;
            current.content += "\n";
        }
    }

    if (in_section)
    {
        current.end_line = lines.size() - 1;
        out.push_back(current);
    }

    return out;
}

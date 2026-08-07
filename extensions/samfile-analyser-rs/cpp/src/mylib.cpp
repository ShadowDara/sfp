#include <mylib.hpp>

void RustMap::insert(rust::Str key, rust::String value)
{
    std::string k(key.data(), key.size());
    map[k] = std::string(value);
}

rust::String RustMap::get(rust::Str key) const
{
    std::string k(key.data(), key.size());

    auto it = map.find(k);

    if (it == map.end())
    {
        return "";
    }

    return it->second;
}

// Function to get the sections
std::unique_ptr<RustMap> get_sections(rust::String value)
{
    RustMap map;

    // Parse the sections
    map.map = parse_Sections(std::string(value));

    return std::make_unique<RustMap>(map);
}

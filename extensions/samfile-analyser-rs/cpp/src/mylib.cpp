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

bool RustMap::rm(rust::Str key)
{
    std::string k(key.data(), key.size());
    return map.erase(k) > 0;
}

size_t RustMap::len() const
{
    return map.size();
}

// for iterator
rust::Vec<rust::String> RustMap::keys() const
{
    rust::Vec<rust::String> result;

    for (const auto& [key, value] : map)
    {
        result.push_back(key);
    }

    return result;
}

// Function to get the sections
std::unique_ptr<RustMap> get_sections(rust::String value)
{
    RustMap map;

    // Parse the sections
    map.map = parse_Sections(std::string(value));

    return std::make_unique<RustMap>(map);
}

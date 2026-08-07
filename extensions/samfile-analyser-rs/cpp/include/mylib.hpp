#pragma once

#include "rust/cxx.h"

#include <string>
#include <unordered_map>
#include <memory>

#include <sfplib/sections.hpp>


// Wrapper for Unordered Map
class RustMap
{
  public:
    std::unordered_map<std::string, std::string> map;

    void insert(rust::Str key, rust::String value);

    rust::String get(rust::Str key) const;
};

// Function to get the sections
std::unique_ptr<RustMap> get_sections(rust::String value);

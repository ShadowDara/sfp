#pragma once

#include "rust/cxx.h"

#include <string>
#include <unordered_map>
#include <memory>
#include <iostream>

#include <sfplib/sections.hpp>


// Wrapper for Unordered Map
class RustMap
{
  public:
    std::unordered_map<std::string, std::string> map;

    void insert(rust::Str key, rust::String value);

    bool rm(rust::Str key);

    rust::String get(rust::Str key) const;

    size_t len() const;

    // for iterator
    rust::Vec<rust::String> keys() const;
};

// Function to get the sections
std::unique_ptr<RustMap> get_sections(rust::String value);

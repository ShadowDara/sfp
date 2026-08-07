// Envirment for the Language

#pragma once

#include <unordered_map>
#include <string>
#include <set>
#include <stdexcept>
#include <memory>
#include <iostream>

#include "values.hpp"


namespace fling::runtime::envirment {
    class Environment;  // Forward Declaration der Klasse

    // Function to setup the Standard Envirment for the Language
    void setupStandardEnvironment(Environment& env);

    class Environment : public std::enable_shared_from_this<Environment> {
    private:
        std::shared_ptr<Environment> parent;  // optional parent (nullptr if none)
        std::unordered_map<std::string, fling::runtime::RuntimeVal> variables;
		std::set<std::string> constants; // Set to track constant variables

    public:
        // Constructor with optional parent
        explicit Environment(std::shared_ptr<Environment> parentEnv = nullptr)
            : parent(std::move(parentEnv))
        {
            if (!parent)
            {
                setupStandardEnvironment(*this);
            }
        }

        // Function to declare a Variable
        fling::runtime::RuntimeVal declareVar(std::string varName,
            fling::runtime::RuntimeVal value, bool constant);

        // Function to Assign a Variable
        fling::runtime::RuntimeVal assignVar(std::string varName,
            fling::runtime::RuntimeVal value);

        // Function to get the Content of a Variable
        fling::runtime::RuntimeVal lookupVar(std::string varName);

        // Function to check if the current Envirment has this Variable
        Environment* resolve(const std::string& varName);

        // Function to check whether a variable exists in this scope or any parent
        bool hasVar(const std::string& varName) const;


        // Delete Copy Constructor and Assignment Operator
        Environment(const Environment&) = delete;
        Environment& operator=(const Environment&) = delete;

        Environment(Environment&&) = default;
        Environment& operator=(Environment&&) = default;

        // Clone function
        inline std::shared_ptr<Environment> clone() const
        {
            auto env = std::make_shared<Environment>(parent);

            env->variables = variables;
            env->constants = constants;

            return env;
        }
    };
}

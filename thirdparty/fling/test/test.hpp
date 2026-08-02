#pragma once

#ifndef TEST_HPP
#define TEST_HPP

#include <iostream>
#include <string>

#include "../src/frontend/parser.hpp"
#include "../src/frontend/lexer.hpp"
#include "../src/frontend/ast.hpp"

#define TEST(name) \
    void name(); \
    int _##name = (run_test(#name, name), 0); \
    void name()

inline void run_test(const std::string& name, void (*func)()) {
    try {
        func();
        std::cout << "[OK] " << name << "\n";
    } catch (...) {
        std::cout << "[FAIL] " << name << "\n";
    }
}

#define EXPECT(cond) if (!(cond)) throw 1

#endif // TEST_HPP

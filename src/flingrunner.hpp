#pragma once

#include <memory>
#include <string>
#include <vector>

#include <frontend/ast.hpp>
#include <frontend/lexer.hpp>
#include <frontend/parser.hpp>
#include <runtime/envirments.hpp>
#include <runtime/interpreter.hpp>
#include <runtime/values.hpp>

void runREPLImGui();

void runFling(const std::string &content);

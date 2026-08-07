#pragma once

#include <string>
#include <fstream>
#include <iostream>

#include "frontend/lexer.hpp"
#include "frontend/parser.hpp"
#include "frontend/ast.hpp"
#include "runtime/interpreter.hpp"
#include "runtime/envirments.hpp"

#include "rust/cxx.h"

// Run the Fling REPL
void runREPL();

// Function to execute a File
void run_file(rust::Str filename);

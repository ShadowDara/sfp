# Fling

<!--

zig c++ .\src\main.cpp .\src\frontend\ast.cpp .\src\frontend\lexer.cpp .\src\frontend\parser.cpp -o fling.exe
.\fling

zig c++ test/test_parser.cpp src/*.cpp -o tests
.\tests

zig c++ test.cpp -o wintest.exe
.\wintest

-->

fling is my first attempt at writing a programming language.
It is a work in progress, and is not yet ready for use.
The language is designed to be simple and easy to learn,
while still being powerful enough to be used for a wide
range of applications, but mainly be used for file
manipulation and scripting.

**BUT The language is still in the early stages of development.**

## TODO

Index Parser
- Parser gets slower over with big lists because O(n) lookup time,
need to implement a hash map for faster lookups.
 
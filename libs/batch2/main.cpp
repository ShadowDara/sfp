#include "batch2.hpp"

#include <string>
#include <iostream>


int main()
{
	std::string input = R"(
:hello

echo Hallo


goto hello

)";

	auto tokens = batch2::tokenize(input);

	std::cout << "Tokens:\n";

	batch2::Interpreter2 interp;

	interp.execute(tokens);

	return 0;
}

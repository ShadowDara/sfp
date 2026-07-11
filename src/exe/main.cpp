#include <sfp/runner.hpp>
#include <iostream>

int main(int argc, char* argv[]) {
	if (argc < 2) {
		std::cerr << "Usage: " << argv[0] << " <input>" << std::endl;
		return 1;
	}
	
	return run_samfile(argv[1], {});
}

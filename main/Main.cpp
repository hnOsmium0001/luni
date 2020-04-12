#include <iostream>
#include <argparse/argparse.hpp>
#include "Util.hpp"

i32 main(i32 argc, char* argv[]) {
	auto argp = argparse::ArgumentParser("LuNI Interpreter");
	argp.add_argument()
		.names({"-l", "--dump-log"})
		.description("Dump lexing, parsing, and code gen logs to stdout.")
		.required(false);
	argp.enable_help();

	auto err = argp.parse(argc, argv);
	if (err) {
		std::cerr << err << '\n';
		return -1;
	}
	
	if (argp.exists("help") {
		argp.print_help();
		return 0;
	}

	return 0;
}

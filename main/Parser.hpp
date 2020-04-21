#pragma once

#include <argparse/argparse.hpp>

namespace LuNI {

class Parser {
private:
	argparse::ArgumentParser* program;

public:
	Parser(argparse::ArgumentParser* program) noexcept;
};

} // namespace LuNI

#pragma once

#include <argparse/argparse.hpp>

namespace LuNI {

class Interpreter {
private:
	argparse::ArgumentParser* program;

public:
	Interpreter(argparse::ArgumentParser* program) noexcept;
};

} // namespace LuNI

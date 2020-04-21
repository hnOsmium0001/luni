#pragma once

#include <argparse/argparse.hpp>

namespace LuNI {

class CodeGen {
private:
	argparse::ArgumentParser* program;

public:
	CodeGen(argparse::ArgumentParser* program) noexcept;
};

} // namespace LuNI

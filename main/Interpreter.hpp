#pragma once

#include <argparse/argparse.hpp>
#include "Program.hpp"

namespace LuNI {

auto RunProgram(
	argparse::ArgumentParser* args,
	BytecodeProgram opcodes
) -> void;

} // namespace LuNI

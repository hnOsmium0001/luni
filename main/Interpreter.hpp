#pragma once

#include "Program.hpp"
#include "Parser.hpp"

#include <argparse/argparse.hpp>

namespace LuNI {

auto RunProgram_WalkAST(
	argparse::ArgumentParser& args,
	const ASTNode& root
) -> void;

auto RunProgram(
	argparse::ArgumentParser& args,
	const BytecodeProgram& opcodes
) -> void;

} // namespace LuNI

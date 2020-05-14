#pragma once

#include <argparse/argparse.hpp>
#include "Program.hpp"
#include "Parser.hpp"

namespace LuNI {

auto RunProgram_WalkAST(
	argparse::ArgumentParser* args,
	ASTNode* root
) -> void;

auto RunProgram(
	argparse::ArgumentParser* args,
	BytecodeProgram opcodes
) -> void;

} // namespace LuNI

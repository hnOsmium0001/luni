#include <stack>
#include "Interpreter.hpp"

using namespace LuNI;

namespace {
class StackFrame {
private:
};
}

auto LuNI::RunProgram_WalkAST(
	argparse::ArgumentParser *args,
	ASTNode *root
) -> void {
	std::stack<StackFrame> callStack;
}

auto LuNI::RunProgram(
	argparse::ArgumentParser* args,
	BytecodeProgram opcodes
) -> void {
	// TODO
}


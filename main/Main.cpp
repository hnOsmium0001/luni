#include <iostream>
#include <fstream>
#include <sstream>
#include <tl/expected.hpp>
#include <argparse/argparse.hpp>
#include "Util.hpp"
#include "Program.hpp"
#include "Parser.hpp"
#include "Interpreter.hpp"

using LuNI::Fmap;
using LuNI::operator|;
using LuNI::Bind;
using LuNI::operator>>;

using namespace LuNI::ErrorCodes;

auto SetupArgParse() -> argparse::ArgumentParser {
	auto program = argparse::ArgumentParser("LuNI Interpreter");
	program.add_argument("inputs")
		.required();
	program.add_argument("--verbose-lexing")
		.help("Output lexing logs along with errors")
		.default_value(false)
		.implicit_value(true);
	program.add_argument("--verbose-parsing")
		.help("Output parsing logs along with errors")
		.default_value(false)
		.implicit_value(true);
	program.add_argument("-b", "--run-bytecode")
		.help("Run the files as bytecode generated by LuNI instead of run them as Lua source code")
		.default_value(false)
		.implicit_value(true);
	program.add_argument("-p", "--output-bytecode")
		.help("Output the generated bytecode to a file named <input-file-name>.luni_bytecode (suffixes will be kept if present)")
		.default_value(false)
		.implicit_value(true);
	return program;
}

auto Err(u32 errorCode, std::string msg) -> tl::expected<LuNI::BytecodeProgram, LuNI::StandardError> {
	return tl::unexpected(LuNI::StandardError{std::move(errorCode), std::move(msg)});
}

auto ProgramFromSource(
	argparse::ArgumentParser& args,
	const std::string& path
) -> tl::expected<LuNI::BytecodeProgram, LuNI::StandardError> {
	auto ifs = std::ifstream{path};
	if (!ifs) {
		return Err(INPUT_FILE_NOT_FOUND, fmt::format("Unable to find source file {}", path));
	}
	auto ifsBuf = std::stringstream{};
	ifsBuf << ifs.rdbuf();
	auto source = ifsBuf.str();

	auto tokens = LuNI::DoLexing(args, source);

#ifdef LUNI_DEBUG_INFO
	for (const auto& token : tokens) {
		fmt::print("[Debug][Lexer] '{}' at {} with type {}\n", token.text, token.pos, token.type);
	}
#endif // #ifdef LUNI_DEBUG_INFO

	auto ast = LuNI::DoParsing(args, tokens);
	auto& astRoot = *ast.root.get();

	LuNI::RunProgram_WalkAST(args, astRoot);

	return LuNI::BytecodeProgram{};
}

auto ProgramFromBytecode(
	argparse::ArgumentParser& args,
	const std::string& path
) -> tl::expected<LuNI::BytecodeProgram, LuNI::StandardError> {
	auto ifs = std::ifstream{path};
	if (!ifs) {
		return Err(INPUT_FILE_NOT_FOUND, fmt::format("Unable to find bytecode file {}", path));
	}

	// TODO
	return LuNI::BytecodeProgram{};
}

int main(int argc, char* argv[]) {
	auto args = SetupArgParse();
	try {
		args.parse_args(argc, argv);
	} catch (const std::runtime_error& err) {
		std::cout << err.what() << '\n';
		std::cout << args;
		return -1;
	}

	auto inputBytecode = args["--run-bytecode"] == true;
	auto inputs = args.get<std::vector<std::string>>("inputs");

	for (const auto& input : inputs) {
		auto res = inputBytecode
			? ProgramFromBytecode(args, input)
			: ProgramFromSource(args, input);
		if (!res) continue;
		auto program = res.value();

		LuNI::RunProgram(args, program);
	}

	return 0;
}


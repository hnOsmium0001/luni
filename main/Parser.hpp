#pragma once

#include <vector>
#include <argparse/argparse.hpp>
#include "Util.hpp"
#include "Lexer.hpp"

namespace LuNI {

class ParsingState {
private:
	std::vector<StandardError> errors;
	// TODO syntax tree
};

auto DoParsing(
	argparse::ArgumentParser* args,
	const LexingState& state
) -> ParsingState;

} // namespace LuNI

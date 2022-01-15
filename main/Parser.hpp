#pragma once

#include "AstNode.hpp"
#include "Error.hpp"
#include "Util.hpp"
#include "fwd.hpp"

#include <fmt/format.h>
#include <argparse/argparse.hpp>
#include <memory>
#include <string>
#include <vector>

namespace LuNI {

struct ParsingResult {
	std::unique_ptr<AstNode> root;
	std::vector<StandardError> errors;
};

auto DoParsing(argparse::ArgumentParser& args, const std::vector<Token>& tokens) -> ParsingResult;

} // namespace LuNI

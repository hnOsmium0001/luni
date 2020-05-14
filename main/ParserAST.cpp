#include <utility>
#include <functional>
#include <initializer_list>
#include <unordered_map>
#include "Parser.hpp"

using namespace LuNI;

auto ASTNode::AddChild(std::unique_ptr<ASTNode> child) -> void {
	children.push_back(std::move(child));
}

namespace {
class ReverseProduction {
public:
	using Productions = std::vector<TokenType>;
	using NonTerminal = std::function<auto() -> std::unique_ptr<ASTNode>>;

	Productions source;
	NonTerminal generation;

	ReverseProduction(NonTerminal generationIn, std::initializer_list<TokenType> sourceIn)
		: source{ sourceIn }, generation{ generationIn } {}

	// TODO
};

class ParsingStack {
public:
	ParsingStack() noexcept = default;
	ParsingStack(const ParsingStack& that) = delete;
	ParsingStack& operator=(const ParsingStack& that) = delete;
	ParsingStack(ParsingStack&& that) = default;
	ParsingStack& operator=(ParsingStack&& that) = default;
};
}

auto DoParsing(
	argparse::ArgumentParser* args,
	const std::vector<Token>& tokens
) -> ParsingResult {
	auto verbose = (*args)["--verbose-parsing"] == true;

	ParsingStack stack;
	// TODO
	return ParsingResult{};
}


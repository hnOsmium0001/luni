#include <utility>
#include <functional>
#include <initializer_list>
#include <unordered_map>
#include "Parser.hpp"

using namespace LuNI;

ASTNode::ASTNode(ASTType type) noexcept
	: type{ type } {}

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

class ParsingState {
public:
	struct Snapshot {
		std::vector<Token>::const_iterator ptr;
	};

public:
	std::unique_ptr<ASTNode> root;
	std::vector<StandardError> errors;
private:
	std::reference_wrapper<const std::vector<Token>> tokens;
	std::vector<Token>::const_iterator ptr;

public:
	using Slice = LuNI::Slice<std::vector<Token>::const_iterator>;

	ParsingState(const std::vector<Token>& tokensIn) noexcept
		: tokens{ std::cref(tokensIn) }, ptr{ tokensIn.begin() } {}

	ParsingState(const ParsingState& that) = delete;
	ParsingState& operator=(const ParsingState& that) = delete;
	ParsingState(ParsingState&& that) = default;
	ParsingState& operator=(ParsingState&& that) = default;

	auto ShouldContinue() const -> bool {
		return this->HasNext(); // TODO
	}

	auto HasNext() const -> bool { return ptr != tokens.get().end(); }

	auto RecordSnapshot() const -> Snapshot {
		return Snapshot{ptr};
	}

	auto RestoreSnapshot(Snapshot snapshot) -> void {
		this->ptr = std::move(snapshot.ptr);
	}

	auto Take() -> const Token* {
		if (!HasNext()) return nullptr;
		auto result = &*ptr;
		++ptr;
		return result;
	}

	auto TakeSome(usize items) -> Slice {
		auto& tokens = this->tokens.get();
		if (!HasNext()) return Slice::None(tokens.end());
		usize dist = std::distance(ptr, tokens.end());
		auto itemsClamped = std::min(dist, items);

		return Slice::At(ptr, itemsClamped);
	}

	/// 将这个ParsingState转换为ParsingResult并非法化任何操作
	/// 注意本函数会将properties移动到返回的ParsingResult内，所以调用此函数之后任何对root AST节点
	/// 或errors列表的操作都会造成UB
	auto FinishParsing() -> ParsingResult {
		return ParsingResult{std::move(this->root), std::move(this->errors)};
	}
};
}

static auto TryMatchExpression(ParsingState& state) -> std::unique_ptr<ASTNode> {
	return nullptr; // TODO
}

static auto TryMatchStatementBlock(ParsingState& state) -> std::unique_ptr<ASTNode> {
	return nullptr; // TODO
}


static auto TryMatchIfStatement(ParsingState& state) -> std::unique_ptr<ASTNode> {
	auto snapshot = state.RecordSnapshot();
	auto snapshotGuard = ScopeGuard([&]() { state.RestoreSnapshot(snapshot); });

	auto ifKW = state.Take();
	if (ifKW->type != TokenType::KEYWORD_IF) return nullptr;

	auto expr = TryMatchExpression(state);
	if (!expr) return nullptr;

	auto thenKW = state.Take();
	if (thenKW->type != TokenType::KEYWORD_THEN) return nullptr;

	auto body = TryMatchStatementBlock(state);
	if (!body) return nullptr;

	auto elseOrEndKW = state.Take();
	switch (elseOrEndKW->type) {
		case TokenType::KEYWORD_ELSE: {
			auto elseBody = TryMatchStatementBlock(state);
			if (!elseBody) return nullptr;

			auto endKW = state.Take();
			if (endKW->type != TokenType::KEYWORD_END) return nullptr;

			// 检测到完整的if-cond-body-else-body
			auto ifNode = std::make_unique<ASTNode>(ASTType::IF);
			ifNode->AddChild(std::move(expr));
			ifNode->AddChild(std::move(body));
			ifNode->AddChild(std::move(elseBody));
			
			snapshotGuard.Cancel();
			return ifNode;
		}
		case TokenType::KEYWORD_END: {
			auto ifNode = std::make_unique<ASTNode>(ASTType::IF);
			ifNode->AddChild(std::move(expr));
			ifNode->AddChild(std::move(body));

			snapshotGuard.Cancel();
			return ifNode;
		}
		default: return nullptr;
	}
}

static auto TryMatchWhileStatement(ParsingState& state) -> bool {
	return false;
}

static auto TryMatchUntilStatement(ParsingState& state) -> bool {
	return false;
}

static auto TryMatchForStatement(ParsingState& state) -> bool {
	return false;
}

static auto TryMatchStatement(ParsingState& state) -> bool {
	if (TryMatchIfStatement(state)) return true;
	if (TryMatchWhileStatement(state)) return true;
	if (TryMatchUntilStatement(state)) return true;
	return false;
}


static auto TryMatchFunctionDefinition(ParsingState& state) -> bool {
	return false;
}

static auto TryMatchDefinition(ParsingState& state) -> bool {
	if (TryMatchFunctionDefinition(state)) return true;
	return false;
}

auto DoParsing(
	argparse::ArgumentParser* args,
	const std::vector<Token>& tokens
) -> ParsingResult {
	auto verbose = (*args)["--verbose-parsing"] == true;

	ParsingState state{tokens};
	while (state.ShouldContinue()) {
	}

	return state.FinishParsing();
}


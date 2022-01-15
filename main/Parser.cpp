#include "Parser.hpp"

#include "Lexer.hpp"
#include "ScopeGuard.hpp"

#include <fmt/core.h>
#include <functional>
#include <iostream>
#include <optional>
#include <span>
#include <stdexcept>
#include <utility>

using namespace LuNI;

static auto PrintNode(const AstNode& node, u32 indent = 0) -> void {
	char buffer[256];
	if (auto& ptr = node.extraData) {
		auto extras = std::visit(
			[&](auto&& value) { return fmt::format("'{}'", value); },
			*ptr);
		fmt::format_to(buffer, "Node(type = {}, extras = {})\n", node.type, extras);
	} else {
		fmt::format_to(buffer, "Node(type = {})\n", node.type);
	}

	fmt::print("{:\t>{}}", buffer, indent);

	for (auto& child : node.children) {
		PrintNode(*child, indent + 1);
	}
}

namespace {
class ParsingState {
public:
	struct Snapshot {
		std::vector<Token>::const_iterator ptr;
	};

	enum class Continuation {
		CONTINUE,
		BREAK_EOF,
		BREAK_NO_CONSUMPTION,
	};

public:
	std::unique_ptr<AstNode> root;
	std::vector<StandardError> errors;

private:
	const std::vector<Token>* tokens;
	std::vector<Token>::const_iterator ptr;
	usize lastIterRemaining = -1;

public:
	ParsingState(const std::vector<Token>& tokensIn) noexcept
		: root{ std::make_unique<AstNode>(AstNode::KD_Script) }
		, errors{}
		, tokens{ &tokensIn }
		, ptr{ tokensIn.begin() } {}

	ParsingState(const ParsingState& that) = delete;
	ParsingState& operator=(const ParsingState& that) = delete;
	ParsingState(ParsingState&& that) = default;
	ParsingState& operator=(ParsingState&& that) = default;

	auto FetchContinuationState() -> Continuation {
		auto remaining = std::distance(ptr, tokens->end());
		if (remaining == lastIterRemaining) {
			return remaining == 0
				? Continuation::BREAK_EOF
				: Continuation::BREAK_NO_CONSUMPTION;
		} else {
			lastIterRemaining = remaining;
			return Continuation::CONTINUE;
		}
	}

	auto HasNext() const -> bool {
		return ptr != tokens->end();
	}

	auto RecordSnapshot() const -> Snapshot {
		return Snapshot{ ptr };
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

	// 当下一个token为指定的类型时返回下一个token，否则返回nullptr
	/// 只有返回非空指针的情况下才会移动`ptr`
	auto TakeIf(TokenType type) -> const Token* {
		if (!HasNext()) return nullptr;
		auto result = &*ptr;
		if (result->type != type) {
			return nullptr;
		} else {
			++ptr;
			return result;
		}
	}

	auto TakeSome(usize items) -> std::span<const Token> {
		if (!HasNext()) return {};
		usize dist = std::distance(ptr, tokens->end());
		auto itemsClamped = std::min(dist, items);

		return std::span<const Token>(ptr, itemsClamped);
	}

	/// 将这个ParsingState转换s移动到返回的ParsingResult内，所以调用此函数之后任何对root AST节点
	/// 或errors列表的操作都会造成UB
	auto FinishParsing() -> ParsingResult {
		return ParsingResult{ std::move(this->root), std::move(this->errors) };
	}
};
} // namespace

// TODO 实现完整的parser backtracking使parser能够在有语法错误的情况下继续工作

static auto TryMatchArrayLiteral(ParsingState& state) -> std::unique_ptr<AstNode> {
	return nullptr; // TODO
}

static auto TryMatchMetatableLiteral(ParsingState& state) -> std::unique_ptr<AstNode> {
	return nullptr; // TODO
}

static auto TryMatchFunctionCall(ParsingState& state) -> std::unique_ptr<AstNode>;
static auto TryMatchExpression(ParsingState& state) -> std::unique_ptr<AstNode> {
	auto snapshot = state.RecordSnapshot();
	SCOPE_GUARD(snapshotGuard) { state.RestoreSnapshot(snapshot); };

	auto first = state.Take();
	if (!first) return nullptr; // EOF
	switch (first->type) {
		case TokenType::STRING_LITERAL: {
			snapshotGuard.Cancel();
			return AstNode::String(first->text);
		}
		case TokenType::INTEGER_LITERAL: {
			snapshotGuard.Cancel();
			return AstNode::Integer(std::stoi(first->text));
		}
		case TokenType::FLOATING_POINT_LITERAL: {
			snapshotGuard.Cancel();
			return AstNode::Float(std::stof(first->text));
		}
		default: {
			// 重置之前那个吃掉的token
			state.RestoreSnapshot(snapshot);
			break;
		}
	}

	auto funcCall = TryMatchFunctionCall(state);
	if (funcCall) {
		snapshotGuard.Cancel();
		return funcCall;
	}

	// TODO

	return nullptr;
}

static auto TryMatchStatement(ParsingState& state) -> std::unique_ptr<AstNode>;
/// 尝试匹配任意数量的语句组合（包括零个）
/// 注意：这意味着该函数必然返回一个非空的AST节点
static auto MatchStatementBlock(ParsingState& state) -> std::unique_ptr<AstNode> {
	auto result = std::make_unique<AstNode>(ASTType::STATEMENT_BLOCK);
	while (true) {
		auto statement = TryMatchStatement(state);
		if (!statement) break;

		result->AddChild(std::move(statement));

		// Lua允许在没有歧义的情况下不包含`end`或者其他结尾关键字之前的分隔符
		// 比如说这是合法的：
		// ```lua
		// function f()
		//     print("hello, world")end
		// --  ^~~~~~~~~~~~~~~~~~~~~ 这是该函数的匹配结果
		// ```
		//
		// 另外Lua也允许在不造成歧义的情况下省略两个语句之间的分割符，比如说：
		// ```lua
		// local a = 0 print("hello, world")print(a)
		// ```
		// 和
		// ```lua
		// local a = 0; print("hello, world")print(a)
		// ```
		// 以及
		// ```lua
		// local a = 0
		// print("hello, world")
		// print(a)
		// ```
		// 是等价的

		state.TakeIf(TokenType::SYMBOL_SEMICOLON);
	}
	return result;
}

static auto TryMatchIfStatement(ParsingState& state) -> std::unique_ptr<AstNode> {
	auto snapshot = state.RecordSnapshot();
	auto snapshotGuard = ScopeGuard([&]() { state.RestoreSnapshot(snapshot); });

	if (!state.TakeIf(TokenType::KEYWORD_IF)) return nullptr;
#ifdef LUNI_DEBUG_INFO
	fmt::print("[Debug][Parser.If] Found keyword 'if'\n");
#endif // #ifdef LUNI_DEBUG_INFO

	auto expr = TryMatchExpression(state);
	if (!expr) return nullptr;
#ifdef LUNI_DEBUG_INFO
	fmt::print("[Debug][Parser.If] Found if conditional (expression node)\n");
#endif // #ifdef LUNI_DEBUG_INFO

	if (!state.TakeIf(TokenType::KEYWORD_THEN)) return nullptr;
#ifdef LUNI_DEBUG_INFO
	fmt::print("[Debug][Parser.If] Found keyword 'then'\n");
#endif // #ifdef LUNI_DEBUG_INFO

	auto body = MatchStatementBlock(state);
#ifdef LUNI_DEBUG_INFO
	fmt::print("[Debug][Parser.If] Found if body (statement block node)\n");
#endif // #ifdef LUNI_DEBUG_INFO

	auto elseOrEndKW = state.Take();
	if (!elseOrEndKW) return nullptr;
	switch (elseOrEndKW->type) {
		case TokenType::KEYWORD_ELSE: {
#ifdef LUNI_DEBUG_INFO
			fmt::print("[Debug][Parser.If] Found keyword 'else'\n");
#endif // #ifdef LUNI_DEBUG_INFO

			auto elseBody = MatchStatementBlock(state);
			if (!elseBody) return nullptr;
#ifdef LUNI_DEBUG_INFO
			fmt::print("[Debug][Parser.If] Found keyword 'end'\n");
#endif // #ifdef LUNI_DEBUG_INFO

			if (!state.TakeIf(TokenType::KEYWORD_END)) return nullptr;
#ifdef LUNI_DEBUG_INFO
			fmt::print("[Debug][Parser.If] Found else body (statement block node)\n");
#endif // #ifdef LUNI_DEBUG_INFO

			// 检测到完整的if-cond-body-else-body
			auto ifNode = std::make_unique<AstNode>(ASTType::IF);
			ifNode->AddChild(std::move(expr));
			ifNode->AddChild(std::move(body));
			ifNode->AddChild(std::move(elseBody));

#ifdef LUNI_DEBUG_INFO
			fmt::print("[Debug][Parser.If] Generated if statement with an else branch\n");
#endif // #ifdef LUNI_DEBUG_INFO

			snapshotGuard.Cancel();
			return ifNode;
		}
		case TokenType::KEYWORD_END: {
#ifdef LUNI_DEBUG_INFO
			fmt::print("[Debug][Parser.If] Found keyword 'end'\n");
#endif // #ifdef LUNI_DEBUG_INFO

			// 不带else语句的if-cond-body
			auto ifNode = std::make_unique<AstNode>(ASTType::IF);
			ifNode->AddChild(std::move(expr));
			ifNode->AddChild(std::move(body));

#ifdef LUNI_DEBUG_INFO
			fmt::print("[Debug][Parser.If] Generated if statement with only `true` branch\n");
#endif // #ifdef LUNI_DEBUG_INFO

			snapshotGuard.Cancel();
			return ifNode;
		}
		default: return nullptr;
	}
}

static auto TryMatchWhileStatement(ParsingState& state) -> std::unique_ptr<AstNode> {
	auto snapshot = state.RecordSnapshot();
	auto snapshotGuard = ScopeGuard([&]() { state.RestoreSnapshot(snapshot); });

	if (!state.TakeIf(TokenType::KEYWORD_WHILE)) return nullptr;

	auto cond = TryMatchExpression(state);
	if (!cond) return nullptr;

	if (!state.TakeIf(TokenType::KEYWORD_DO)) return nullptr;

	auto body = MatchStatementBlock(state);

	if (!state.TakeIf(TokenType::KEYWORD_END)) return nullptr;

	auto whileNode = std::make_unique<AstNode>(ASTType::WHILE);
	whileNode->AddChild(std::move(cond));
	whileNode->AddChild(std::move(body));

	snapshotGuard.Cancel();
	return whileNode;
}

static auto TryMatchUntilStatement(ParsingState& state) -> std::unique_ptr<AstNode> {
	auto snapshot = state.RecordSnapshot();
	auto snapshotGuard = ScopeGuard([&]() { state.RestoreSnapshot(snapshot); });

	if (!state.TakeIf(TokenType::KEYWORD_REPEAT)) return nullptr;

	auto body = MatchStatementBlock(state);

	if (!state.TakeIf(TokenType::KEYWORD_UNTIL)) return nullptr;

	auto cond = TryMatchExpression(state);
	if (!cond) return nullptr;

	auto untilNode = std::make_unique<AstNode>(ASTType::UNTIL);
	untilNode->AddChild(std::move(cond));
	untilNode->AddChild(std::move(body));

	snapshotGuard.Cancel();
	return untilNode;
}

static auto TryMatchForStatement(ParsingState& state) -> std::unique_ptr<AstNode> {
	auto snapshot = state.RecordSnapshot();
	auto snapshotGuard = ScopeGuard([&]() { state.RestoreSnapshot(snapshot); });

	if (!state.TakeIf(TokenType::KEYWORD_FOR)) return nullptr;

	auto varName = state.TakeIf(TokenType::IDENTIFIER);
	if (!varName) return nullptr;

	if (state.Take()->type != TokenType::KEYWORD_IN) return nullptr;

	std::array<std::unique_ptr<AstNode>, 3> exprs;
	{
		for (usize i = 0; i < 2; ++i) {
			exprs[i] = TryMatchExpression(state);
			if (!exprs[i]) return nullptr;

			if (!state.TakeIf(TokenType::SYMBOL_COMMA)) return nullptr;
		}

		exprs[2] = TryMatchExpression(state);
		if (!exprs[2]) {
			exprs[2] = std::make_unique<AstNode>(ASTType::INTEGER_LITERAL);
			// 第三个参数默认为1
			// 不手动指定u32的话编译器会无法完成int->(unsigned int->u32)->std::variant的隐式转换
			exprs[2]->SetExtraData(u32{ 1 });
		}
	}

	auto body = MatchStatementBlock(state);

	if (!state.TakeIf(TokenType::KEYWORD_END)) return nullptr;

	auto forNode = std::make_unique<AstNode>(ASTType::FOR);
	forNode->AddChild(AstNode::Identifier(varName->text));
	// 使用ranged-based for loops的话似乎不能从数组里move出来，只能复制或者取引用
	forNode->AddChild(std::move(exprs[0]));
	forNode->AddChild(std::move(exprs[1]));
	forNode->AddChild(std::move(exprs[2]));
	forNode->AddChild(std::move(body));

	snapshotGuard.Cancel();
	return forNode;
}

static auto TryMatchVariableDeclaration(ParsingState& state) -> std::unique_ptr<AstNode> {
	auto snapshot = state.RecordSnapshot();
	auto snapshotGuard = ScopeGuard([&]() { state.RestoreSnapshot(snapshot); });

	// 可选local修饰符
	auto type = state.TakeIf(TokenType::KEYWORD_LOCAL) == nullptr
		? ASTType::VARIABLE_DECLARATION
		: ASTType::LOCAL_VARIABLE_DECLARATION;

	auto name = state.TakeIf(TokenType::IDENTIFIER);
	if (!name) return nullptr;

	if (!state.TakeIf(TokenType::OPERATOR_ASSIGN)) return nullptr;

	auto expr = TryMatchExpression(state);
	if (!expr) return nullptr;

	auto varDec = std::make_unique<AstNode>(type);
	varDec->AddChild(AstNode::Identifier(name->text));
	varDec->AddChild(std::move(expr));

	snapshotGuard.Cancel();
	return varDec;
}

static auto MatchFunctionParams(ParsingState& state) -> std::unique_ptr<AstNode> {
	auto params = std::make_unique<AstNode>(ASTType::FUNCTION_CALL_PARAMS);
	while (true) {
		auto param = TryMatchExpression(state);
		if (!param) break;

		params->AddChild(std::move(param));

		// Lua允许trailing commas
		// 如果没有逗号了，那么当前参数列表肯定已经结束了（或者是语法错误）
		// 如果还有逗号并且下次迭代时没有可用的表达式（参数）了，那么当前参数列表也会正常结束
		if (!state.TakeIf(TokenType::SYMBOL_COMMA)) break;
	}
	return params;
}

static auto TryMatchFunctionCall(ParsingState& state) -> std::unique_ptr<AstNode> {
	auto snapshot = state.RecordSnapshot();
	auto snapshotGuard = ScopeGuard([&]() { state.RestoreSnapshot(snapshot); });

	auto funcName = state.TakeIf(TokenType::IDENTIFIER);
	if (!funcName) return nullptr;

	if (!state.TakeIf(TokenType::SYMBOL_LEFT_PAREN)) return nullptr;
	// 参数列表永远返回一个非空指针（因为参数列表可空）
	auto paramList = MatchFunctionParams(state);
	if (!state.TakeIf(TokenType::SYMBOL_RIGHT_PAREN)) return nullptr;

	auto funcCall = std::make_unique<AstNode>(ASTType::FUNCTION_CALL);
	funcCall->AddChild(AstNode::Identifier(funcName->text));
	funcCall->AddChild(std::move(paramList));

	snapshotGuard.Cancel();
	return funcCall;
}

static auto TryMatchStatement(ParsingState& state) -> std::unique_ptr<AstNode> {
	auto funcCall = TryMatchFunctionCall(state);
	if (funcCall) return funcCall;

	auto ifSt = TryMatchIfStatement(state);
	if (ifSt) return ifSt;

	auto whileSt = TryMatchWhileStatement(state);
	if (whileSt) return whileSt;

	auto untilSt = TryMatchUntilStatement(state);
	if (untilSt) return untilSt;

	auto forSt = TryMatchUntilStatement(state);
	if (forSt) return forSt;

	auto varDec = TryMatchVariableDeclaration(state);
	if (varDec) return varDec;

	return nullptr;
}

static auto MatchFunctionDefParams(ParsingState& state) -> std::unique_ptr<AstNode> {
	auto params = std::make_unique<AstNode>(ASTType::FUNCTION_CALL_PARAMS);
	while (true) {
		auto param = state.TakeIf(TokenType::IDENTIFIER);
		if (!param) break;

		params->AddChild(AstNode::Identifier(param->text));

		// Lua允许trailing commas
		if (!state.TakeIf(TokenType::SYMBOL_COMMA)) break;
	}
	return params;
}

static auto TryMatchFunctionDefinition(ParsingState& state) -> std::unique_ptr<AstNode> {
	auto snapshot = state.RecordSnapshot();
	auto snapshotGuard = ScopeGuard([&]() { state.RestoreSnapshot(snapshot); });

	if (!state.TakeIf(TokenType::KEYWORD_FUNCTION)) return nullptr;

	auto name = state.TakeIf(TokenType::IDENTIFIER);
	if (!name) return nullptr;

	if (!state.TakeIf(TokenType::SYMBOL_LEFT_PAREN)) return nullptr;
	auto params = MatchFunctionDefParams(state);
	if (!state.TakeIf(TokenType::SYMBOL_RIGHT_PAREN)) return nullptr;

	auto body = MatchStatementBlock(state);

	if (!state.TakeIf(TokenType::KEYWORD_END)) return nullptr;

	auto funcDef = std::make_unique<AstNode>(ASTType::FUNCTION_DEFINITION);
	funcDef->AddChild(AstNode::Identifier(name->text));
	funcDef->AddChild(std::move(params));
	funcDef->AddChild(std::move(body));

	snapshotGuard.Cancel();
	return funcDef;
}

static auto TryMatchDefinition(ParsingState& state) -> std::unique_ptr<AstNode> {
	auto functionDef = TryMatchFunctionDefinition(state);
	if (functionDef) return functionDef;

	return nullptr;
}

auto LuNI::DoParsing(
	argparse::ArgumentParser& args,
	const std::vector<Token>& tokens) -> ParsingResult {
	auto verbose = args["--verbose-parsing"] == true;

	ParsingState state{ tokens };
	while (true) {
		auto cont = state.FetchContinuationState();
		switch (cont) {
			case ParsingState::Continuation::CONTINUE:
				break;
			case ParsingState::Continuation::BREAK_EOF:
				if (verbose) fmt::print("[Parser] Reached end of file when parsing, finishing normally");
				goto end;
			case ParsingState::Continuation::BREAK_NO_CONSUMPTION:
				if (verbose) fmt::print("[Parser] No tokens are able to be consumed, finishing with error");
				goto end;
		}

		if (auto node = TryMatchDefinition(state)) {
			if (verbose) {
				fmt::print("Collected top-level definition:\n");
				PrintNode(*node);
				fmt::print("\n");
				// fmt::print("Tok: {}\n", state.Take()->text);
			}
			state.root->AddChild(std::move(node));
			continue;
		}
		if (auto node = TryMatchStatement(state)) {
			if (verbose) {
				fmt::print("Collected top-level statement:\n");
				PrintNode(*node);
				fmt::print("\n");
			}
			state.root->AddChild(std::move(node));
			continue;
		}
	}

end:
	return state.FinishParsing();
}

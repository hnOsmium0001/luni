#include <stdexcept>
#include <utility>
#include <functional>
#include <initializer_list>
#include <unordered_map>
#include "Parser.hpp"

using namespace LuNI;

auto ASTNode::Identifier(std::string text) -> std::unique_ptr<ASTNode> {
	auto res = std::make_unique<ASTNode>(ASTType::IDENTIFIER);
	res->SetExtraData(std::move(text));
	return res;
}

ASTNode::ASTNode(ASTType type) noexcept
	: type{ type } {}

auto ASTNode::AddChild(std::unique_ptr<ASTNode> child) -> void {
	children.push_back(std::move(child));
}

auto ASTNode::SetExtraData(ExtraData data) -> void {
	if (!this->extraData) {
		this->extraData = std::make_unique<ASTNode::ExtraData>(std::move(data));
	} else {
		*this->extraData = std::move(data);
	}
}

auto ASTNode::GetExtraData() const -> const ExtraData& {
	if (!extraData) {
		throw std::runtime_error("Trying to get reference to null extraData!");
	} else {
		return *extraData;
	}
}
auto ASTNode::GetExtraData() -> ExtraData& {
	return const_cast<ExtraData&>(static_cast<const ASTNode&>(*this).GetExtraData());
}

namespace {
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

	auto TakeIdentifier() -> const Token* {
		if (!HasNext()) return nullptr;
		auto token = &*ptr;
		++ptr;
		if (token->type != TokenType::IDENTIFIER) {
			return nullptr;
		} else {
			return token;
		}
	}

	/// 当下一个token为指定的类型时返回下一个token，否则返回nullptr
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

// TODO 实现完整的parser backtracking使parser能够在有语法错误的情况下继续工作

static auto TryMatchExpression(ParsingState& state) -> std::unique_ptr<ASTNode> {
	return nullptr; // TODO
}

// 垃圾c++不能自动循环引用函数
static auto TryMatchStatement(ParsingState& state) -> std::unique_ptr<ASTNode>;
/// 尝试匹配任意数量的语句组合（包括零个）
/// 注意：这意味着该函数必然返回一个非空的AST节点
static auto MatchStatementBlock(ParsingState& state) -> std::unique_ptr<ASTNode> {
	auto result = std::make_unique<ASTNode>(ASTType::STATEMENT_BLOCK);
	while (true) {
		auto statement = TryMatchStatement(state);
		if (statement == nullptr) break;

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
		// print("hello, world")print(a)
		// ```
		// 是等价的

		state.TakeIf(TokenType::WHITESPACE);
		state.TakeIf(TokenType::SYMBOL_SEMICOLON);
	}
	return result;
}


static auto TryMatchIfStatement(ParsingState& state) -> std::unique_ptr<ASTNode> {
	auto snapshot = state.RecordSnapshot();
	auto snapshotGuard = ScopeGuard([&]() { state.RestoreSnapshot(snapshot); });

	if (!state.TakeIf(TokenType::KEYWORD_IF)) return nullptr;

	auto expr = TryMatchExpression(state);
	if (!expr) return nullptr;

	if (!state.TakeIf(TokenType::KEYWORD_THEN)) return nullptr;

	auto body = MatchStatementBlock(state);

	auto elseOrEndKW = state.Take();
	if (!elseOrEndKW) return nullptr;
	switch (elseOrEndKW->type) {
		case TokenType::KEYWORD_ELSE: {
			auto elseBody = MatchStatementBlock(state);
			if (!elseBody) return nullptr;

			if (!state.TakeIf(TokenType::KEYWORD_END)) return nullptr;

			// 检测到完整的if-cond-body-else-body
			auto ifNode = std::make_unique<ASTNode>(ASTType::IF);
			ifNode->AddChild(std::move(expr));
			ifNode->AddChild(std::move(body));
			ifNode->AddChild(std::move(elseBody));
			
			snapshotGuard.Cancel();
			return ifNode;
		}
		case TokenType::KEYWORD_END: {
			if (!state.TakeIf(TokenType::KEYWORD_END)) return nullptr;

			// 不带else语句的if-cond-body
			auto ifNode = std::make_unique<ASTNode>(ASTType::IF);
			ifNode->AddChild(std::move(expr));
			ifNode->AddChild(std::move(body));

			snapshotGuard.Cancel();
			return ifNode;
		}
		default: return nullptr;
	}
}

static auto TryMatchWhileStatement(ParsingState& state) -> std::unique_ptr<ASTNode> {
	auto snapshot = state.RecordSnapshot();
	auto snapshotGuard = ScopeGuard([&]() { state.RestoreSnapshot(snapshot); });

	if (!state.TakeIf(TokenType::KEYWORD_WHILE)) return nullptr;

	auto cond = TryMatchExpression(state);
	if (!cond) return nullptr;

	if (!state.TakeIf(TokenType::KEYWORD_DO)) return nullptr;

	auto body = MatchStatementBlock(state);

	if (!state.TakeIf(TokenType::KEYWORD_END)) return nullptr;

	auto whileNode = std::make_unique<ASTNode>(ASTType::WHILE);
	whileNode->AddChild(std::move(cond));
	whileNode->AddChild(std::move(body));

	snapshotGuard.Cancel();
	return whileNode;
}

static auto TryMatchUntilStatement(ParsingState& state) -> std::unique_ptr<ASTNode> {
	auto snapshot = state.RecordSnapshot();
	auto snapshotGuard = ScopeGuard([&]() { state.RestoreSnapshot(snapshot); });

	if (!state.TakeIf(TokenType::KEYWORD_REPEAT)) return nullptr;

	auto body = MatchStatementBlock(state);

	if (!state.TakeIf(TokenType::KEYWORD_UNTIL)) return nullptr;

	auto cond = TryMatchExpression(state);
	if (!cond) return nullptr;

	auto untilNode = std::make_unique<ASTNode>(ASTType::UNTIL);
	untilNode->AddChild(std::move(cond));
	untilNode->AddChild(std::move(body));

	snapshotGuard.Cancel();
	return untilNode;
}

static auto TryMatchForStatement(ParsingState& state) -> std::unique_ptr<ASTNode> {
	auto snapshot = state.RecordSnapshot();
	auto snapshotGuard = ScopeGuard([&]() { state.RestoreSnapshot(snapshot); });

	if (!state.TakeIf(TokenType::KEYWORD_FOR)) return nullptr;

	auto varName = state.TakeIdentifier();
	if (!varName) return nullptr;

	if (state.Take()->type != TokenType::KEYWORD_IN) return nullptr;

	std::array<std::unique_ptr<ASTNode>, 3> exprs;
	{
		for (usize i = 0; i < 2; ++i) {
			exprs[i] = TryMatchExpression(state);
			if (!exprs[i]) return nullptr;

			if (!state.TakeIf(TokenType::SYMBOL_COMMA)) return nullptr;
		}

		exprs[2] = TryMatchExpression(state);
		if (!exprs[2]) {
			exprs[2] = std::make_unique<ASTNode>(ASTType::INTEGER_LITERAL);
			// 第三个参数默认为1
			// 不手动指定u32的话编译器会无法完成int->(unsigned int->u32)->std::variant的隐式转换
			exprs[2]->SetExtraData(u32{1});
		}
	}

	auto body = MatchStatementBlock(state);

	if (!state.TakeIf(TokenType::KEYWORD_END)) return nullptr;

	auto forNode = std::make_unique<ASTNode>(ASTType::FOR);
	forNode->AddChild(ASTNode::Identifier(varName->text));
	// 使用ranged-based for loops的话似乎不能从数组里move出来，只能复制或者取引用
	forNode->AddChild(std::move(exprs[0]));
	forNode->AddChild(std::move(exprs[1]));
	forNode->AddChild(std::move(exprs[2]));
	forNode->AddChild(std::move(body));

	snapshotGuard.Cancel();
	return forNode;
}

static auto TryMatchVariableDeclaration(ParsingState& state) -> std::unique_ptr<ASTNode> {
	auto snapshot = state.RecordSnapshot();
	auto snapshotGuard = ScopeGuard([&]() { state.RestoreSnapshot(snapshot); });

	auto type = state.TakeIf(TokenType::KEYWORD_LOCAL) == nullptr
		? ASTType::VARIABLE_DECLARATION
		: ASTType::LOCAL_VARIABLE_DECLARATION;

	auto name = state.TakeIdentifier();
	if (!name) return nullptr;

	if (!state.TakeIf(TokenType::OPERATOR_ASSIGN)) return nullptr;

	auto expr = TryMatchExpression(state);
	if (!expr) return nullptr;

	auto varDec = std::make_unique<ASTNode>(type);
	varDec->AddChild(ASTNode::Identifier(name->text));
	varDec->AddChild(std::move(expr));

	snapshotGuard.Cancel();
	return varDec;
}

static auto MatchFunctionParams(ParsingState& state) -> std::unique_ptr<ASTNode> {
	auto params = std::make_unique<ASTNode>(ASTType::FUNCTION_CALL_PARAMS);
	while (true) {
		auto param = TryMatchExpression(state);
		if (!param) break;

		params->AddChild(std::move(param));

		// 本lua实现允许trailing commas（其实主要是不允许的话实现起来比较麻烦233）
		// 如果没有逗号了，那么当前参数列表肯定已经结束了（或者是语法错误）
		// 如果还有逗号并且下次迭代时没有可用的表达式（参数）了，那么当前参数列表也会正常结束
		if (!state.TakeIf(TokenType::SYMBOL_COMMA)) break;
	}
	return params;
}

static auto TryMatchFunctionCall(ParsingState& state) -> std::unique_ptr<ASTNode> {
	auto snapshot = state.RecordSnapshot();
	auto snapshotGuard = ScopeGuard([&]() { state.RestoreSnapshot(snapshot); });
	
	auto funcName = state.TakeIdentifier();
	if (!funcName) return nullptr;

	if (!state.TakeIf(TokenType::SYMBOL_LEFT_PAREN)) return nullptr;
	// 参数列表永远返回一个非空指针（因为参数列表可空）
	auto paramList = MatchFunctionParams(state);
	if (!state.TakeIf(TokenType::SYMBOL_RIGHT_PAREN)) return nullptr;

	auto funcCall = std::make_unique<ASTNode>(ASTType::FUNCTION_CALL);
	funcCall->AddChild(ASTNode::Identifier(funcName->text));
	funcCall->AddChild(std::move(paramList));
	return funcCall;
}

static auto TryMatchStatement(ParsingState& state) -> std::unique_ptr<ASTNode> {
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

	auto funcCall = TryMatchFunctionCall(state);
	if (funcCall) return funcCall;

	return nullptr;
}


static auto MatchFunctionDefParams(ParsingState& state) -> std::unique_ptr<ASTNode> {
	// TODO
	return nullptr;
}

static auto TryMatchFunctionDefinition(ParsingState& state) -> std::unique_ptr<ASTNode> {
	// TODO
	return nullptr;
}

static auto TryMatchDefinition(ParsingState& state) -> std::unique_ptr<ASTNode> {
	auto functionDef = TryMatchFunctionDefinition(state);
	if (functionDef) return functionDef;

	return nullptr;
}

auto LuNI::DoParsing(
	argparse::ArgumentParser* args,
	const std::vector<Token>& tokens
) -> ParsingResult {
	auto verbose = (*args)["--verbose-parsing"] == true;

	ParsingState state{tokens};
	while (state.ShouldContinue()) {
		// TODO
	}

	return state.FinishParsing();
}


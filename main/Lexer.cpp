#include "Lexer.hpp"

#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <algorithm>
#include <functional>
#include <magic_enum.hpp>
#include <regex>
#include <stdexcept>
#include <unordered_set>
#include <utility>

using namespace LuNI;

static auto fisrtKeywordID = static_cast<u32>(TokenType::KEYWORD_AND);
static auto lastKeywordID = static_cast<u32>(TokenType::KEYWORD_WHILE);
static auto firstOperID = static_cast<u32>(TokenType::OPERATOR_PLUS);
static auto lastOperID = static_cast<u32>(TokenType::SYMBOL_3_DOT);
auto LuNI::NormalizeTokenType(TokenType type) -> TokenType {
	auto id = static_cast<u32>(type);
	if (id >= fisrtKeywordID && id <= lastKeywordID) {
		return TokenType::KEYWORD;
	} else if (id >= firstOperID && id <= lastOperID) {
		return TokenType::OPERATOR;
	} else {
		return type;
	}
}

auto LuNI::StringifyTokenType(TokenType type) -> std::string_view {
	switch (type) {
		case TokenType::KEYWORD_AND: return "and";
		case TokenType::KEYWORD_BREAK: return "break";
		case TokenType::KEYWORD_DO: return "do";
		case TokenType::KEYWORD_ELSE: return "else";
		case TokenType::KEYWORD_ELSEIF: return "elseif";
		case TokenType::KEYWORD_END: return "end";
		case TokenType::KEYWORD_FALSE: return "false";
		case TokenType::KEYWORD_FOR: return "for";
		case TokenType::KEYWORD_FUNCTION: return "function";
		case TokenType::KEYWORD_IF: return "if";
		case TokenType::KEYWORD_IN: return "in";
		case TokenType::KEYWORD_LOCAL: return "local";
		case TokenType::KEYWORD_NIL: return "nil";
		case TokenType::KEYWORD_NOT: return "not";
		case TokenType::KEYWORD_OR: return "or";
		case TokenType::KEYWORD_REPEAT: return "repeat";
		case TokenType::KEYWORD_RETURN: return "return";
		case TokenType::KEYWORD_THEN: return "then";
		case TokenType::KEYWORD_TRUE: return "true";
		case TokenType::KEYWORD_UNTIL: return "until";
		case TokenType::KEYWORD_WHILE: return "while";
		case TokenType::OPERATOR_PLUS: return "+";
		case TokenType::OPERATOR_MINUS: return "-";
		case TokenType::OPERATOR_MULTIPLY: return "*";
		case TokenType::OPERATOR_DIVIDE: return "/";
		case TokenType::OPERATOR_MOD: return "%";
		case TokenType::OPERATOR_EXPONENT: return "^";
		case TokenType::OPERATOR_LENGTH: return "#";
		case TokenType::OPERATOR_EQUALS: return "==";
		case TokenType::OPERATOR_NOT_EQUAL: return "~=";
		case TokenType::OPERATOR_LESS_EQ: return "<=";
		case TokenType::OPERATOR_GREATER_EQ: return ">=";
		case TokenType::OPERATOR_LESS: return "<";
		case TokenType::OPERATOR_GREATER: return ">";
		case TokenType::OPERATOR_ASSIGN: return "=";
		case TokenType::SYMBOL_LEFT_PAREN: return "(";
		case TokenType::SYMBOL_RIGHT_PAREN: return ")";
		case TokenType::SYMBOL_LEFT_BRACE: return "{";
		case TokenType::SYMBOL_RIGHT_BRACE: return "}";
		case TokenType::SYMBOL_LEFT_BRACKET: return "[";
		case TokenType::SYMBOL_RIGHT_BRACKET: return "]";
		// 分号在lua里没什么卵用，就是让lexer正确分割token而已
		// 实际上（TODO 看下specs）跟空白符一模一样
		case TokenType::SYMBOL_SEMICOLON: return ";";
		case TokenType::SYMBOL_COLON: return ":";
		case TokenType::SYMBOL_COMMA: return ",";
		case TokenType::SYMBOL_DOT: return ".";
		case TokenType::SYMBOL_2_DOT: return "..";
		case TokenType::SYMBOL_3_DOT: return "...";

		case TokenType::IDENTIFIER: return "identifier";
		case TokenType::KEYWORD: return "keyword";
		case TokenType::OPERATOR: return "operator";
		case TokenType::STRING_LITERAL: return "string literal";
		case TokenType::INTEGER_LITERAL: return "integer literal";
		case TokenType::FLOATING_POINT_LITERAL: return "floating point literal";

		default: return "unknown token type";
	}
}

namespace {
class LexingState {
public:
	std::vector<Token> tokens;

private:
	const std::string* src;
	std::string::const_iterator ptr;
	u32 currentLine = 1;
	u32 currentColumn = 1;

public:
	LexingState(const std::string& src) noexcept
		: src{ &src }, ptr{ src.cbegin() } {
	}

	LexingState(const LexingState&) = delete;
	LexingState& operator=(const LexingState&) = delete;
	LexingState(LexingState&&) = default;
	LexingState& operator=(LexingState&&) = default;

	auto HasNext() const -> bool {
		return ptr != src->cend();
	}

	auto Peek(usize offset = 0) const -> std::optional<char> {
		if (!HasNext()) return {};
		auto ptr = this->ptr + offset;
		return *ptr;
	}

	auto PeekSome(usize chars, usize offset = 0) const -> std::optional<std::string_view> {
		if (!HasNext()) return {};
		auto ptr = this->ptr + offset;

		// 从std::string::const_iterator::difference_type强制装换成usize
		// 不知为何std::distance模板参数推导失败，得显示声明模板参数才行
		usize dist = std::distance<std::string::const_iterator>(ptr, src->cend());
		auto charsClamped = std::min(dist, chars);
		return std::string_view(&*ptr, charsClamped);
	}

	auto Take() -> std::optional<char> {
		if (!HasNext()) return {};
		auto result = *ptr;
		++ptr;

		if (result == '\n') {
			this->currentColumn = 1;
			++this->currentLine;
		} else {
			++this->currentColumn;
		}
		return result;
	}

	auto TakeSome(usize chars) -> std::optional<std::string_view> {
		if (!HasNext()) return {};
		usize dist = std::distance(ptr, src->cend());
		auto charsClamped = std::min(dist, chars);

		// 将std::string::iterator转换为指针
		auto result = std::string_view(&*ptr, charsClamped);

		// TODO optimize?
		for (usize i = 0; i < charsClamped; ++i) {
			auto c = *ptr;
			if (c == '\n') {
				this->currentColumn = 1;
				++this->currentLine;
			} else {
				++this->currentColumn;
			}
			++ptr;
		}

		return result;
	}

	auto Advance() -> bool {
		if (!HasNext()) {
			return false;
		} else {
			if (*ptr == '\n') {
				this->currentColumn = 1;
				++this->currentLine;
			} else {
				++this->currentColumn;
			}
			++ptr;
			return true;
		}
	}

	auto Advance(usize chars) -> usize {
		usize dist = std::distance(ptr, src->cend());
		auto charsClamped = std::min(dist, chars);

		// TODO optimize?
		for (usize i = 0; i < charsClamped; ++i) {
			auto c = *ptr;
			if (c == '\n') {
				this->currentColumn = 1;
				++this->currentLine;
			} else {
				++this->currentColumn;
			}
			++ptr;
		}

		return charsClamped;
	}

	auto Previous() -> bool {
		if (ptr == src->cbegin()) {
			return false;
		} else {
			--ptr;

			if (*ptr == '\n') {
				this->currentColumn = 1;
				++this->currentLine;
			} else {
				++this->currentColumn;
			}
			return true;
		}
	}

	auto Previous(usize chars) -> usize {
		usize dist = std::distance(src->cbegin(), ptr);
		auto charsClamped = std::min(dist, chars);

		// TODO optimize?
		for (usize i = 0; i < charsClamped; ++i) {
			--ptr;
			auto c = *ptr;
			if (c == '\n') {
				this->currentColumn = 1;
				++this->currentLine;
			} else {
				++this->currentColumn;
			}
		}

		return charsClamped;
	}

	auto AddToken(Token token) -> void {
		tokens.push_back(token);
	}

	auto GetLastToken() const -> const Token& {
		return tokens.back();
	}

	u32 line() const { return currentLine; }
	u32 column() const { return currentColumn; }
};
} // namespace

static auto CurrentPosOf(const LexingState& state) -> TokenPos {
	return TokenPos{ state.line(), state.column() };
}

// TODO 把上面那个巨型switch换成bimap
static const std::unordered_map<std::string_view, TokenType> keywords{
	{ "and", TokenType::KEYWORD_AND },
	{ "break", TokenType::KEYWORD_BREAK },
	{ "do", TokenType::KEYWORD_DO },
	{ "else", TokenType::KEYWORD_ELSE },
	{ "elseif", TokenType::KEYWORD_ELSEIF },
	{ "end", TokenType::KEYWORD_END },
	{ "false", TokenType::KEYWORD_FALSE },
	{ "for", TokenType::KEYWORD_FOR },
	{ "function", TokenType::KEYWORD_FUNCTION },
	{ "if", TokenType::KEYWORD_IF },
	{ "in", TokenType::KEYWORD_IN },
	{ "local", TokenType::KEYWORD_LOCAL },
	{ "nil", TokenType::KEYWORD_NIL },
	{ "not", TokenType::KEYWORD_NOT },
	{ "or", TokenType::KEYWORD_OR },
	{ "repeat", TokenType::KEYWORD_REPEAT },
	{ "return", TokenType::KEYWORD_RETURN },
	{ "then", TokenType::KEYWORD_THEN },
	{ "true", TokenType::KEYWORD_TRUE },
	{ "until", TokenType::KEYWORD_UNTIL },
	{ "while", TokenType::KEYWORD_WHILE },
};
static const std::unordered_map<std::string_view, TokenType> operators{
	{ "+", TokenType::OPERATOR_PLUS },
	{ "-", TokenType::OPERATOR_MINUS },
	{ "*", TokenType::OPERATOR_MULTIPLY },
	{ "/", TokenType::OPERATOR_DIVIDE },
	{ "%", TokenType::OPERATOR_MOD },
	{ "^", TokenType::OPERATOR_EXPONENT },
	{ "#", TokenType::OPERATOR_LENGTH },
	{ "==", TokenType::OPERATOR_EQUALS },
	{ "~=", TokenType::OPERATOR_NOT_EQUAL },
	{ "<=", TokenType::OPERATOR_LESS_EQ },
	{ ">=", TokenType::OPERATOR_GREATER_EQ },
	{ "<", TokenType::OPERATOR_LESS },
	{ ">", TokenType::OPERATOR_GREATER },
	{ "=", TokenType::OPERATOR_ASSIGN },
	{ "(", TokenType::SYMBOL_LEFT_PAREN },
	{ ")", TokenType::SYMBOL_RIGHT_PAREN },
	{ "{", TokenType::SYMBOL_LEFT_BRACE },
	{ "}", TokenType::SYMBOL_RIGHT_BRACE },
	{ "[", TokenType::SYMBOL_LEFT_BRACKET },
	{ "]", TokenType::SYMBOL_RIGHT_BRACKET },
	{ ";", TokenType::SYMBOL_SEMICOLON },
	{ ":", TokenType::SYMBOL_COLON },
	{ ",", TokenType::SYMBOL_COMMA },
	{ ".", TokenType::SYMBOL_DOT },
	{ "..", TokenType::SYMBOL_2_DOT },
	{ "...", TokenType::SYMBOL_3_DOT },
};
static const std::regex identifierBegin("[a-zA-Z_]");
static const std::regex identifierAfter("[0-9a-zA-Z_]");
static const std::regex integerLiteral("[0-9]");
static const std::regex floatPointLiteral("[0-9]*\\.[0-9]+");
static const std::string_view lineComment = "--";
static const std::string_view blockCommentBeg = "--[[";
static const std::string_view blockCommentEnd = "--]]";

static auto TryLexIdentifierOrKeyword(LexingState& state) -> std::optional<Token> {
	auto beginning = state.Peek().value_or(' ');
	spdlog::trace("[Debug][Lexer.Iden] First char: '{}'\n", beginning);

	if (!std::regex_match(std::string{ beginning }, identifierBegin)) {
		spdlog::trace("[Debug][Lexer.Iden] First char cannot be a part of an identifier, returning\n");
		return {};
	}
	// 为了简化逻辑所以不直接使用Peek到的第一个字符，而是从头开始一个个地Take

	// 接下来必然是identifier
	auto pos = CurrentPosOf(state);

	std::string buf;
	while (true) {
		auto opt = state.Take();
		if (!opt) break;

		spdlog::trace("[Debug][Lexer.Iden] Fetched char: '{}'\n", *opt);

		if (!std::regex_match(std::string{ *opt }, identifierAfter)) {
			spdlog::trace("[Debug][Lexer.Iden] Fetched char cannot be a part of an identifier, result: '{}'\n", buf);

			state.Previous();
			break;
		}

		buf += *opt;
		spdlog::trace("[Debug][Lexer.Iden] Current buffer: '{}'\n", buf);
	}

	auto it = keywords.find(buf);
	auto type = it == keywords.end()
		? TokenType::IDENTIFIER
		: it->second;

	return Token{ std::move(buf), std::move(pos), type };
}

static auto TryLexOperator(LexingState& state) -> std::optional<Token> {
	// 优先检测较长的运算符（“>”）以确保较短的运算符不会吞掉较长运算符的头部分
	// 例：“<=”有可能会被当作“<"
	// 注释以“--”开头且没有任何运算符以其结尾，所以不用管错误匹配到注释的一部分
	for (usize n = 3; n >= 1; --n) {
		auto opt = state.PeekSome(n);
		if (!opt) return {};
		auto view = *opt;

		spdlog::trace("[Debug][Lexer.Oper] Matching length {}, input: '{}'\n", n, view);

		auto it = operators.find(view);
		if (it != operators.end()) {
			if (it->second == TokenType::SYMBOL_SEMICOLON) {
				continue;
			}

			spdlog::trace("[Debug][Lexer.Oper] Found matching operator '{}' with length {}\n", view, n);

			auto pos = CurrentPosOf(state);
			state.Advance(n);
			return Token{ std::string{ view }, std::move(pos), it->second };
		}

		spdlog::trace("[Debug][Lexer.Oper] No matching operator found with length {}\n", n);
	}

	spdlog::trace("[Debug][Lexer.Oper] No matching operator found with either lengths, aborting\n");
	return {};
}

static auto TryLexSimpleString(LexingState& state) -> std::string {
	std::string buf;
	while (true) {
		auto opt = state.Take();
		if (!opt) break;
		auto c = *opt;

		spdlog::trace("[Debug][Lexer.Str] Fetched char '{}'\n", c);

		if (c == '"') {
			spdlog::trace("[Debug][Lexer.Str] Found string literal ending, result: '{}'\n", buf);
			break;
		}

		buf += c;
		spdlog::trace("[Debug][Lexer.Str] Current buffer: '{}'\n", buf);
	}

	return buf;
}

static auto TryLexMultilineString(LexingState& state) -> std::string {
	std::string buf;
	// TODO
	return buf;
}

static auto TryLexString(LexingState& state) -> std::optional<Token> {
	if (state.Peek().value_or(' ') == '"') {
		spdlog::trace("[Debug][Lexer.Str] Found string literal beginning\n");

		state.Advance();
		auto pos = CurrentPosOf(state);
		return Token{ TryLexSimpleString(state), std::move(pos), TokenType::STRING_LITERAL };
	}

	if (state.PeekSome(2).value_or("") == "[[") {
		spdlog::trace("[Debug][Lexer.Str] Found multiline string literal beginning\n");

		state.Advance(2);
		auto pos = CurrentPosOf(state);
		return Token{ TryLexMultilineString(state), std::move(pos), TokenType::STRING_LITERAL };
	}

	spdlog::trace("[Debug][Lexer.Str] No string literal beginning found\n");
	return {};
}

// TODO 添加二进制、十六进制字面量的支持
static auto TryLexIntegerLiteral(LexingState& state) -> std::optional<Token> {
	auto firstOpt = state.Peek();
	if (!firstOpt) return {};
	char beginning = *firstOpt;

	spdlog::trace("[Debug][Lexer.Int] First char: '{}'\n", beginning);

	if (!std::regex_match(std::string{ beginning }, integerLiteral)) {
		spdlog::trace("[Debug][Lexer.Int] First char cannot be a part of an integer literal, returning\n");
		return {};
	}

	auto pos = CurrentPosOf(state);

	std::string buf;
	while (true) {
		auto opt = state.Take();
		if (!opt) break;

		spdlog::trace("[Debug][Lexer.Int] Fetched char: '{}'\n", *opt);

		if (!std::regex_match(std::string{ *opt }, integerLiteral)) {
			spdlog::trace("[Debug][Lexer.Int] Fetched char cannot be a part of an integer literal, result: '{}'\n", buf);

			state.Previous();
			break;
		}

		buf += *opt;
		spdlog::trace("[Debug][Lexer.Int] Current buffer: '{}'\n", buf);
	}

	return Token{ std::move(buf), std::move(pos), TokenType::INTEGER_LITERAL };
}

static auto TryLexFloatingPointLiteral(LexingState& state) -> std::optional<Token> {
	auto front = TryLexIntegerLiteral(state);
	return {};
}

static auto TryLexLineComment(LexingState& state) -> void {
	// 销毁所有字符直到一个换行符
	while (true) {
		auto opt = state.Take();
		if (!opt) return;
		auto c = *opt;
		if (c == '\n') return;
	}
}

static auto TryLexMultilineComment(LexingState& state) -> void {
	// 销毁所有字符直到“--]]”
	u32 stage = 0;
	while (true) {
		auto opt = state.Take();
		if (!opt) return;
		auto c = *opt;

		switch (c) {
			case '-': {
				if (stage < 2) ++stage;
				break;
			}
			case ']': {
				if (stage == 2 && stage < 4) {
					++stage;
				} else if (stage == 4) {
					return;
				}
				break;
			}
			default: {
				stage = 0;
				break;
			}
		}
	}
}

static auto TryLexComments(LexingState& state) -> std::optional<TokenPos> {
	auto first2Opt = state.PeekSome(2);
	if (!first2Opt) return {};
	auto first2 = *first2Opt;

	// block comment marks also starts with --
	if (first2 != lineComment) {
		spdlog::trace("[Debug][Lexer.Comment] No comment beginning found\n");
		return {};
	}

	// From this point we know we have a comment
	auto pos = CurrentPosOf(state);
	state.Advance(2);

	auto next2 = state.PeekSome(2, 2);
	if (next2 && *next2 == "[[") {
		spdlog::trace("[Debug][Lexer.Comment] Found multiline comment beginning\n");

		state.Advance(2);
		TryLexMultilineComment(state);
	} else {
		spdlog::trace("[Debug][Lexer.Comment] Found line comment beginning\n");

		TryLexLineComment(state);
	}
	return pos;
}

auto LuNI::DoLexing(argparse::ArgumentParser& args, const std::string& text) -> std::vector<Token> {
	auto verbose = args["--verbose-lexing"] == true;

	LexingState state{ text };
	while (state.HasNext()) {
		auto iden = TryLexIdentifierOrKeyword(state);
		if (iden) {
			spdlog::info("[Lexer] Generated {} token '{}'\n", magic_enum::enum_name(iden->type), iden->text);
			spdlog::info("\tstarting at {}\n", iden->pos);
			state.AddToken(std::move(*iden));
			continue;
		}

		auto str = TryLexString(state);
		if (str) {
			spdlog::info("[Lexer] Generated string literal token '{}'\n", str->text);
			spdlog::info("\tstarting at {}\n", str->pos);
			state.AddToken(std::move(*str));
			continue;
		}

		auto commentPos = TryLexComments(state);
		if (commentPos) {
			spdlog::info("[Lexer] Discarded comments starting at {}\n", *commentPos);
			continue;
		}

		auto oper = TryLexOperator(state);
		if (oper) {
			spdlog::info("[Lexer] Generated operator token '{}'\n", oper->text);
			spdlog::info("\tstarting at {}\n", oper->pos);
			state.AddToken(std::move(*oper));
			continue;
		}

		auto nextChar = state.Peek().value_or('\0');
		if (std::isspace(nextChar)) {
#if 0
			auto hasWhitepsace = state.GetLastToken().type == TokenType::WHITESPACE;
			// 因为AST生成器会检查换行符（“\n”），所以必须保留所有的换行符
			// 为了简化AST生成器的逻辑（不需要处处检查并吞掉空白符），其他的空白符就不保留了
			// 只要能够正常分离不同的token就行了
			if (nextChar != '\n') {
				spdlog::info("Discarded whitespace at {}\n", CurrentPosOf(state));
			} else if (!hasWhitepsace) {
				// 同样是为了简化AST生成器的工作，只保留一个换行符
				auto currentPos = CurrentPosOf(state);
				spdlog::info("Generated whitespace token at {}\n", currentPos);
				state.AddToken(Token{std::string{nextChar}, currentPos, TokenType::WHITESPACE});
			}
#endif
			spdlog::info("[Lexer] Discarded whitespace at {}\n", CurrentPosOf(state));
			state.Advance();
			continue;
		}
	}
	return state.tokens;
}

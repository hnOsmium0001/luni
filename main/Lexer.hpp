#pragma once

#include "Error.hpp"
#include "Util.hpp"
#include "fwd.hpp"

#include <fmt/format.h>
#include <argparse/argparse.hpp>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace LuNI {

/// 表示token类型的enum。包含基础类型以及特殊类型（关键字、运算符，符号等）。
///
/// 不包含注释token，所有的注释都会在lexing阶段被剔除。
enum class TokenType {
	// ======== 基础类型token ========

	IDENTIFIER,
	KEYWORD,
	OPERATOR,

	INTEGER_LITERAL,
	FLOATING_POINT_LITERAL,
	STRING_LITERAL,

	// ========= 特殊类型token ========
	// 包含关键字、运算符，符号等

	KEYWORD_AND,
	KEYWORD_BREAK,
	KEYWORD_DO,
	KEYWORD_ELSE,
	KEYWORD_ELSEIF,
	KEYWORD_END,
	KEYWORD_FALSE,
	KEYWORD_FOR,
	KEYWORD_FUNCTION,
	KEYWORD_IF,
	KEYWORD_IN,
	KEYWORD_LOCAL,
	KEYWORD_NIL,
	KEYWORD_NOT,
	KEYWORD_OR,
	KEYWORD_REPEAT,
	KEYWORD_RETURN,
	KEYWORD_THEN,
	KEYWORD_TRUE,
	KEYWORD_UNTIL,
	KEYWORD_WHILE,

	OPERATOR_PLUS, // "+"
	OPERATOR_MINUS, // "-"
	OPERATOR_MULTIPLY, // "*"
	OPERATOR_DIVIDE, // "/"
	OPERATOR_MOD, // "%"
	OPERATOR_EXPONENT, // "^"
	OPERATOR_LENGTH, // "#"
	OPERATOR_EQUALS, // "=="
	OPERATOR_NOT_EQUAL, // "~="
	OPERATOR_LESS_EQ, // "<="
	OPERATOR_GREATER_EQ, // ">="
	OPERATOR_LESS, // "<"
	OPERATOR_GREATER, // ">"
	OPERATOR_ASSIGN, // "="

	// “符号”（名词）大致就是不能转换成函数的符号（字面意思）
	// 注意在lexer中“符号”和运算符都被放在`operators`下
	SYMBOL_LEFT_PAREN, // "("
	SYMBOL_RIGHT_PAREN, // ")"
	SYMBOL_LEFT_BRACE, // "{"
	SYMBOL_RIGHT_BRACE, // "}"
	SYMBOL_LEFT_BRACKET, // "["
	SYMBOL_RIGHT_BRACKET, // "]"
	SYMBOL_SEMICOLON, // ";"
	SYMBOL_COLON, // ":"
	SYMBOL_COMMA, // ","
	SYMBOL_DOT, // "."
	SYMBOL_2_DOT, // ".."
	SYMBOL_3_DOT, // "..."
};

auto NormalizeTokenType(TokenType type) -> TokenType;
auto StringifyTokenType(TokenType type) -> std::string_view;

struct TokenPos {
	u32 line;
	u32 column;
};

struct Token {
	std::string text;
	TokenPos pos;
	TokenType type;
};

auto DoLexing(argparse::ArgumentParser& args, const std::string& text) -> std::vector<Token>;

} // namespace LuNI

template <>
struct fmt::formatter<LuNI::TokenPos> {
	template <typename ParseContext>
	constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

	template <typename FormatContext>
	auto format(const LuNI::TokenPos& pos, FormatContext& ctx) {
		return format_to(ctx.out(), "{}:{}", pos.line, pos.column);
	}
};

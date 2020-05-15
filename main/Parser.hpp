#pragma once

#include <string>
#include <optional>
#include <vector>
#include <memory>
#include <fmt/format.h>
#include <argparse/argparse.hpp>
#include "Util.hpp"

namespace LuNI {

/// 表示token类型的enum。包含基础类型以及特殊类型（关键字、运算符，符号等）。
/// 
/// 不包含注释token，所有的注释都会在lexing阶段被剔除。
enum class TokenType {
	// ======== 基础类型token ========

	WHITESPACE,

	IDENTIFIER,
	KEYWORD,
	OPERATOR,
	
	STRING_LITERAL,
	MULTILINE_STRING_LITERAL,

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

auto FormatTokenType(TokenType type) -> std::string_view;
auto NormalizeTokenType(TokenType type) -> TokenType;

struct TokenPos {
	u32 line;
	u32 column;
};

struct Token {
	std::string text;
	TokenPos pos;
	TokenType type;
};

enum class ASTType {
	FUNCTION_DECLARATION,

	PARAMETER_LIST, // 只包含标识符
	PARAMETER,

	IF,
	WHILE,
	UNTIL,
	FOR,
	VARIABLE_DECLARATION,

	IDENTIFIER,
	

	FUNCTION_CALL_PARAMS, // 任意表达式（标识符也算）
	FUNCTION_CALL, // 既可以是表达式也可以是语句
};

class ASTNode {
public:
	ASTType type;
	std::vector<std::unique_ptr<ASTNode>> children;

public:
	ASTNode(ASTType type) noexcept;
	~ASTNode() noexcept = default;

	ASTNode(ASTNode&& that) noexcept = default;
	ASTNode& operator=(ASTNode&& that) noexcept = default;
	ASTNode(const ASTNode& that) noexcept = default;
	ASTNode& operator=(const ASTNode& that) noexcept = default;

	auto AddChild(std::unique_ptr<ASTNode> child) -> void;
};

class ParsingResult {
public:
	std::unique_ptr<ASTNode> root;
private:
	std::vector<StandardError> errors;

public:
	ParsingResult(std::unique_ptr<ASTNode> root, std::vector<StandardError> errors) noexcept
		: root{ std::move(root) }, errors{ std::move(errors) } {}
	~ParsingResult() noexcept = default;

	ParsingResult(const ParsingResult& that) noexcept = delete;
	ParsingResult& operator=(const ParsingResult& that) noexcept = delete;
	ParsingResult(ParsingResult&& that) noexcept = default;
	ParsingResult& operator=(ParsingResult&& that) noexcept = default;
};


auto DoParsing(
	argparse::ArgumentParser* args,
	const std::vector<Token>& tokens
) -> ParsingResult;

auto DoLexing(
	argparse::ArgumentParser* args,
	const std::string& text
) -> std::vector<Token>;

} // namespace LuNI

template <>
struct fmt::formatter<LuNI::TokenType> {
	template <typename ParseContext>
	constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

	template <typename FormatContext>
	auto format(const LuNI::TokenType& type, FormatContext& ctx) {
		using LuNI::TokenType;
		switch (LuNI::NormalizeTokenType(type)) {
			case TokenType::WHITESPACE: return format_to(ctx.out(), "whitespace");
			case TokenType::IDENTIFIER: return format_to(ctx.out(), "identifier");
			case TokenType::KEYWORD: return format_to(ctx.out(), "keyword");
			case TokenType::OPERATOR: return format_to(ctx.out(), "operator");
			case TokenType::STRING_LITERAL: return format_to(ctx.out(), "string literal");
			case TokenType::MULTILINE_STRING_LITERAL: return format_to(ctx.out(), "multiline string literal");
		}
		throw std::runtime_error("Invalid token type");
	}
};

template <>
struct fmt::formatter<LuNI::TokenPos> {
	constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

	template <typename FormatContext>
	auto format(const LuNI::TokenPos& pos, FormatContext& ctx) {
		return format_to(ctx.out(), "{}:{}", pos.line, pos.column);
	}
};

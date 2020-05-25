#pragma once

#include <string>
#include <optional>
#include <variant>
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
auto Format(TokenType type) -> std::string_view;

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
	SCRIPT,

	INTEGER_LITERAL,
	FLOATING_POINT_LITERAL,
	STRING_LITERAL,
	ARRAY_LITERAL,
	METATABLE_LITERAL,

	FUNCTION_DEFINITION,

	FUNC_DEF_PARAMETER_LIST, // 只包含标识符
	FUNC_DEF_PARAMETER,

	IF,
	WHILE,
	UNTIL,
	FOR,
	LOCAL_VARIABLE_DECLARATION,
	VARIABLE_DECLARATION,
	// 上方节点的任意组合，包括FUNCTION_CALL
	STATEMENT_BLOCK,

	IDENTIFIER,

	FUNCTION_CALL_PARAMS, // 任意表达式（标识符也算）
	FUNCTION_CALL, // 既可以是表达式也可以是语句
};

auto Format(ASTType type) -> std::string_view;

class ASTNode {
public:
	// 所有的子类型必须支持fmt::format
	using ExtraData = std::variant<
		u32,
		f32,
		std::string
		// TODO lua array
		// TODO lua metatable
	>;

	ASTType type;
	std::vector<std::unique_ptr<ASTNode>> children;
	std::unique_ptr<ExtraData> extraData;

public:
	static auto Identifier(std::string text) -> std::unique_ptr<ASTNode>;
	static auto Integer(u32 literal) -> std::unique_ptr<ASTNode>;
	static auto Float(f32 literal) -> std::unique_ptr<ASTNode>;
	static auto String(std::string literal) -> std::unique_ptr<ASTNode>;
	// TODO array
	// TODO metatable

	ASTNode(ASTType type) noexcept;
	~ASTNode() noexcept = default;

	ASTNode(ASTNode&& that) noexcept = default;
	ASTNode& operator=(ASTNode&& that) noexcept = default;
	ASTNode(const ASTNode& that) noexcept = default;
	ASTNode& operator=(const ASTNode& that) noexcept = default;

	auto AddChild(std::unique_ptr<ASTNode> child) -> void;

	auto SetExtraData(ExtraData data) -> void;
	/// 断言extraData非空
	auto GetExtraData() const -> const ExtraData&;
	/// 断言extraData非空
	auto GetExtraData() -> ExtraData&;
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
	argparse::ArgumentParser& args,
	const std::vector<Token>& tokens
) -> ParsingResult;

auto DoLexing(
	argparse::ArgumentParser& args,
	const std::string& text
) -> std::vector<Token>;

} // namespace LuNI

template <>
struct fmt::formatter<LuNI::TokenType> {
	template <typename ParseContext>
	constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

	template <typename FormatContext>
	auto format(const LuNI::TokenType& type, FormatContext& ctx) {
		return format_to(ctx.out(), LuNI::Format(type));
	}
};

template <>
struct fmt::formatter<LuNI::TokenPos> {
	template <typename ParseContext>
	constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

	template <typename FormatContext>
	auto format(const LuNI::TokenPos& pos, FormatContext& ctx) {
		return format_to(ctx.out(), "{}:{}", pos.line, pos.column);
	}
};

template <>
struct fmt::formatter<LuNI::ASTType> {
	template <typename ParseContext>
	constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

	template <typename FormatContext>
	auto format(const LuNI::ASTType& type, FormatContext& ctx) {
		return format_to(ctx.out(), LuNI::Format(type));
	}
};


#pragma once

#include <string>
#include <optional>
#include <vector>
#include <fmt/format.h>
#include <argparse/argparse.hpp>
#include "Util.hpp"

namespace LuNI {

class LexingState;
class TokenPos {
public:
	u32 line;
	u32 column;

	static auto CurrentPos(const LexingState& state) -> TokenPos;
};

enum class TokenType {
	IDENTIFIER,
	KEYWORD,
	OPERATOR,

	// There is no comment tokens, comments are stripped out at the lexing stage
	/*COMMENT_BEGIN,
	MULTILINE_COMMENT_BEGIN,
	MULTILINE_COMMENT_END,*/
	
	STRING_LITERAL,
	MULTILINE_STRING_LITERAL,
};

class Token {
public:
	std::string text;
	TokenPos pos;
	TokenType type;
};

class LexingState {
public:
	std::vector<Token> tokens;
private:
	std::string src;
	std::string::iterator ptr;
	u32 currentLine = 0;
	u32 currentColumn = 0;

public:
	LexingState(std::string src) noexcept;
	LexingState(const LexingState&) = delete;
	LexingState& operator=(const LexingState&) = delete;
	LexingState(LexingState&&) = default;
	LexingState& operator=(LexingState&&) = default;

	auto HasNext() const -> bool;
	auto Peek(usize offset = 0) -> std::optional<char>;
	auto PeekSome(usize chars, usize offset = 0) -> std::optional<std::string_view>;
	auto PeekFull(usize chars, usize offset = 0) -> std::optional<std::string_view>;
	auto Take() -> std::optional<char>;
	auto TakeSome(usize chars) -> std::optional<std::string_view>;
	auto Advance() -> bool;
	auto Advance(usize chars) -> usize;
	auto Previous() -> bool;
	auto Previous(usize chars) -> usize;

	auto AddToken(Token token) -> void;
	auto GetToken(usize i) -> Token&;
	auto GetToken(usize i) const -> const Token&;

	u32 line() const { return currentLine; }
	u32 column() const { return currentColumn; }
};

class Lexer {
private:
	argparse::ArgumentParser* program;

public:
	Lexer(argparse::ArgumentParser* program) noexcept;

	auto DoLexing(std::string text) -> LexingState;
};

} // namespace LuNI

template <> struct fmt::formatter<LuNI::TokenType>;
std::ostream& operator<<(std::ostream& out, const LuNI::TokenType& type);
template <> struct fmt::formatter<LuNI::TokenPos>;
std::ostream& operator<<(std::ostream& out, const LuNI::TokenPos& pos);

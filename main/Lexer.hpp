#pragma once

#include <string>
#include <optional>
#include <vector>
#include "Util.hpp"

namespace LuNI {

enum class TokenType {
	IDENTIFIER,
	KEYWORD,
	OPERATOR,

	// There is no comment tokens, comments are stripped out at the lexing stage
	//COMMENT_BEGIN,
	//MULTILINE_COMMENT_BEGIN,
	//MULTILINE_COMMENT_END,
	
	STRING_LITERAL,
};

class Token {
public:
	std::string text;
	TokenType type;
};

class LexingState {
public:
	std::vector<Token> tokens;
private:
	std::string src;
	std::string::iterator ptr;

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
};

auto LexInput(std::string text) -> LexingState;

} // namespace LuNI

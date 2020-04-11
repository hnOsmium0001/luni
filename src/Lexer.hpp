#pragma once

#include <string>
#include <optional>
#include <vector>
#include "Util.hpp"

namespace LuNI {

enum class TokenType {
	// TODO
};

class Token {
public:
	std::string text;
	TokenType type;
};

class LexingState {
private:
	std::vector<Token> tokens;
	std::string src;
	std::string::iterator ptr;

public:
	LexingState(std::string src) noexcept;
	LexingState(const LexingState&) = delete;
	LexingState& operator=(const LexingState&) = delete;
	LexingState(LexingState&&) = default;
	LexingState& operator=(LexingState&&) = default;

	auto Peek() -> std::optional<char>;
	auto Peek(usize chars) -> std::optional<std::string_view>;
	auto Take() -> std::optional<char>;
	auto Take(usize chars) -> std::optional<std::string_view>;
	auto TakeCopy(usize chars) -> std::optional<std::string>;

	auto AddToken(Token token) -> void;
};

} // namespace LuNI

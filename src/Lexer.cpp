#include <algorithm>
#include "Lexer.hpp"

using namespace LuNI;

LexingState::LexingState(std::string src) noexcept
	: src{ std::move(src) }, ptr{ this->src.begin() } {
}

auto LexingState::Peek() -> std::optional<char> {
	if (ptr == src.end()) return {};
	return *ptr;
}
auto LexingState::Peek(usize chars) -> std::optional<std::string_view> {
	if (ptr == src.end()) return {};
	// Force convert to usize (from iterator::difference_type)
	usize dist = std::distance(ptr, src.end());
	auto charsClamped = std::min(dist, chars);
	return std::string_view(&*ptr, charsClamped);
}
auto LexingState::Take() -> std::optional<char> {
	if (ptr == src.end()) return {};
	auto result = *ptr;
	++ptr;
	return result;
}
auto LexingState::Take(usize chars) -> std::optional<std::string_view> {
	if (ptr == src.end()) return {};
	usize dist = std::distance(ptr, src.end());
	auto charsClamped = std::min(dist, chars);
	auto result = std::string_view(&*ptr, charsClamped);
	std::advance(ptr, charsClamped);
	return result;
}
auto LexingState::TakeCopy(usize chars) -> std::optional<std::string> {
	if (ptr == src.end()) return {};
	return Fmap(
			[](auto view) { return std::string{view}; },
			std::move(Take(chars))
	);
}

auto LexingState::AddToken(Token token) -> void {
	tokens.push_back(token);
}

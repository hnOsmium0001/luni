#include <utility>
#include <functional>
#include <algorithm>
#include <regex>
#include <unordered_set>
#include "Lexer.hpp"

using namespace LuNI;

LexingState::LexingState(std::string src) noexcept
	: src{ std::move(src) }, ptr{ this->src.begin() } {
}

auto LexingState::HasNext() const -> bool {
	return ptr == src.end();
}
auto LexingState::Peek(usize offset) -> std::optional<char> {
	auto ptr = this->ptr + offset;
	if (!HasNext()) return {};
	return *ptr;
}
auto LexingState::PeekSome(usize chars, usize offset) -> std::optional<std::string_view> {
	auto ptr = this->ptr + offset;
	if (!HasNext()) return {};
	// Force convert to usize (from iterator::difference_type)
	usize dist = std::distance(ptr, src.end());
	auto charsClamped = std::min(dist, chars);
	return std::string_view(&*ptr, charsClamped);
}
auto LexingState::PeekFull(usize chars, usize offset) -> std::optional<std::string_view> {
	auto ptr = this->ptr + offset;
	if (!HasNext()) return {};
	usize dist = std::distance(ptr, src.end());
	if (dist < chars) {
		return {};
	} else {
		return std::string_view{&*ptr, chars};
	}
}
auto LexingState::Take() -> std::optional<char> {
	if (!HasNext()) return {};
	auto result = *ptr;
	++ptr;
	return result;
}
auto LexingState::TakeSome(usize chars) -> std::optional<std::string_view> {
	if (!HasNext()) return {};
	usize dist = std::distance(ptr, src.end());
	auto charsClamped = std::min(dist, chars);
	auto result = std::string_view(&*ptr, charsClamped);
	std::advance(ptr, charsClamped);
	return result;
}
auto LexingState::Advance() -> bool {
	if (!HasNext()) {
		return false;
	} else {
		++ptr;
		return true;
	}
}
auto LexingState::Advance(usize chars) -> usize {
	usize dist = std::distance(ptr, src.end());
	auto c = std::min(dist, chars);
	std::advance(ptr, c);
	return c;
}
auto LexingState::Previous() -> bool {
	if (ptr == src.begin()) {
		return false;
	} else {
		--ptr;
		return true;
	}
}
auto LexingState::Previous(usize chars) -> usize {
	usize dist = std::distance(src.begin(), ptr);
	auto c = std::min(dist, chars);
	std::advance(ptr, -c);
	return c;
}

auto LexingState::AddToken(Token token) -> void {
	tokens.push_back(token);
}
auto LexingState::GetToken(usize i) -> Token& {
	return tokens[i];
}
auto LexingState::GetToken(usize i) const -> const Token& {
	return tokens[i];
}

namespace {
	static const std::unordered_set<std::string_view> keywords{
		"and", "break",		 "do",		 "else", "elseif", "end",		"false",
		"for", "function", "if",		 "in",	 "local",  "nil",		"not",
		"or",  "repeat",	 "return", "then", "true",	 "until", "while",
	};
	static const std::unordered_set<std::string_view> operators{
		"+", "-", "*", "/", "%", "^", "#", "==", "~=", "<=", ">=", "<",  ">",
		"=", "(", ")", "{", "}", "[", "]", ";",  ":",  ",",  ".",  "..", "...",
	};
	static const std::regex identifierBegin("[a-zA-Z_]");
	static const std::regex identifierAfter("[0-9a-zA-Z_]+");
	static const std::string_view lineComment = "--";
	static const std::string_view blockCommentBeg = "--[[";
	static const std::string_view blockCommentEnd = "--]]";

	// These functions will assume the LexingState have at least one more char to go
	auto TryLexIdentifier(LexingState& state) -> std::optional<Token> {
		auto beginning = state.Peek().value();
		if (!std::regex_match(std::string{beginning}, identifierBegin)) return {};
		// We know this must be an identifier (or keyword) at this point
		std::string buf;
		while (true) {
			auto opt = state.Take();
			if (!opt) break;
			if (std::regex_match(std::string{*opt}, identifierAfter)) {
				buf += *opt;
			} else {
				state.Previous();
				break;
			}
		}
		return Token{std::move(buf), TokenType::IDENTIFIER};
	}

	auto TryLexOperator(LexingState& state) -> std::optional<Token> {
		// Iterate backwards to prevent shorter operators (e.g. ">") break the
		// actual longer operator (e.g. ">="
		for (usize i = 3; i >= 1; --i) {
			auto opt = state.PeekSome(i);
			if (!opt) return {};
			std::string_view view = *opt;
			if (operators.find(view) != operators.end()) {
				state.Advance(i);
				return Token{std::string{view}, TokenType::OPERATOR};
			}
		}
		return {};
	}

	auto TryLexSimpleString(LexingState& state) -> std::string {
		std::string buf;
		while (true) {
			auto opt = state.Take();
			if (!opt) break; 
			auto c = *opt;
			if (c == '"') {
				break;
			} else {
				buf += c;
			}
		}
		return buf;
	}
	auto TryLexMultilineString(LexingState& state) -> std::string {
		// TODO
		//// Number of equal signs between the brackets
		//u32 equals = 0;
		//while (true) {
		//  auto opt = state.Take();
		//  if (!opt) return {};
		//  auto c = *opt;
		//  switch (c) {
		//    case '=':
		//      ++equals;
		//      break;
		//    case ']': 
		//      goto contentReading;
		//    default:
		//      // Not a sring literal, probably an operator instead
		//      return {};
		//  }
		//}
		
		contentReading:
		std::string buf;
		// TODO
		return buf;
	}
	auto TryLexString(LexingState& state) -> std::optional<Token> {
		auto first = state.Peek().value();
		if (first == '"') {
			return Token{TryLexSimpleString(state), TokenType::STRING_LITERAL};
		}
		if (first == '[') {
			return Token{TryLexMultilineString(state), TokenType::STRING_LITERAL};
		}
		return {};
	}

	auto TryLexLineComment(LexingState& state) -> void {
		// Keep consuming input until we see a line break
		while (true) {
			auto opt = state.Take();
			if (!opt) return;
			auto c = *opt;
			if (c == '\n') return;
		}
	}
	auto TryLexMultilineComment(LexingState& state) -> void {
		// TODO
		//// Keep consuming input until we see a multiline comment end
		//u32 stage = 0;
		//while (true) {
		//  auto opt = state.Take();
		//  if (!opt) return true;
		//  auto c = *opt;
		//  switch (c) {
		//    case '-':
		//      ++stage;
		//      break;
		//    case ']':
		//      // In case there is things like "--...--]" in the comment,
		//      // stage will be above 2
		//      if (stage >= 2) {
		//        return;
		//      }
		//      break;
		//    default:
		//      stage = 0;
		//      break;
		//  }
		//}
	}
	auto TryLexComments(LexingState& state) -> bool {
		auto first2 = state.PeekFull(2).value();
		// block comment marks also starts with --
		if (first2 != lineComment) return false;
		// From this point we know we have a comment

		auto third = state.Peek(3);
		if (third && *third == '[') {
		} else {
			TryLexLineComment(state);
		}
		return true;
	}
}
auto LexInput(std::string text) -> LexingState {
	LexingState state{std::move(text)};
	while (state.HasNext()) {
		auto iden = TryLexIdentifier(state);
		if (iden) {
			auto it = keywords.find(iden->text);
			if (it != keywords.end()) iden->type = TokenType::KEYWORD;
			state.AddToken(std::move(*iden));
			continue;
		}

		auto str = TryLexString(state);
		if (str) {
			state.AddToken(std::move(*str));
			continue;
		}

		auto commentSuccess = TryLexComments(state);
		if (commentSuccess) {
			continue;
		}

		auto oper = TryLexOperator(state);
		if (oper) {
			state.AddToken(std::move(*oper));
			continue;
		}

		if (std::isspace(state.Peek().value_or('\0'))) {
			state.Advance();
			continue;
		}
	}
	return state;
}

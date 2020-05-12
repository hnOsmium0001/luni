// #define LUNI_DEBUG_INFO

#include <utility>
#include <functional>
#include <algorithm>
#include <regex>
#include <unordered_set>
#include <stdexcept>
#include "Lexer.hpp"

using namespace LuNI;

auto TokenPos::CurrentPos(const LexingState &state) -> TokenPos {
	return TokenPos{state.line(), state.column()};
}

LexingState::LexingState(std::string src) noexcept
	: src{ std::move(src) }, ptr{ this->src.begin() } {
}

auto LexingState::HasNext() const -> bool {
	return ptr != src.end();
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
	static const std::regex identifierAfter("[0-9a-zA-Z_]");
	static const std::string_view lineComment = "--";
	static const std::string_view blockCommentBeg = "--[[";
	static const std::string_view blockCommentEnd = "--]]";

	auto TryLexIdentifier(LexingState& state) -> std::optional<Token> {
		auto beginning = state.Peek().value_or(' ');
#ifdef LUNI_DEBUG_INFO
		fmt::print("[Debug][Lexer.Iden] First char: '{}'\n", beginning);
#endif // #ifdef LUNI_DEBUG_INFO

		if (!std::regex_match(std::string{beginning}, identifierBegin)) {
#ifdef LUNI_DEBUG_INFO
			fmt::print("[Debug][Lexer.Iden] First char cannot be a part of an identifier, returning\n");
#endif // #ifdef LUNI_DEBUG_INFO
			return {};
		}

		// We know this must be an identifier (or keyword) at this point
		auto pos = TokenPos::CurrentPos(state);

		std::string buf;
		while (true) {
			auto opt = state.Take();
			if (!opt) break;
#ifdef LUNI_DEBUG_INFO
		fmt::print("[Debug][Lexer.Iden] Fetched char: '{}'\n", *opt);
#endif // #ifdef LUNI_DEBUG_INFO

			if (!std::regex_match(std::string{*opt}, identifierAfter)) {
				state.Previous();
#ifdef LUNI_DEBUG_INFO
		fmt::print("[Debug][Lexer.Iden] Fetched char cannot be a part of an identifier, result: '{}'\n", buf);
#endif // #ifdef LUNI_DEBUG_INFO
				break;
			}

			buf += *opt;
#ifdef LUNI_DEBUG_INFO
		fmt::print("[Debug][Lexer.Iden] Current buffer: '{}'\n", buf);
#endif // #ifdef LUNI_DEBUG_INFO
		}
		return Token{std::move(buf), pos, TokenType::IDENTIFIER};
	}

	auto TryLexOperator(LexingState& state) -> std::optional<Token> {
		// Iterate backwards to prevent shorter operators (e.g. ">") break the
		// actual longer operator (e.g. ">="
		// No need to worry about accidentally matching comments immediately after
		// comments, no operators end eith a "-"
		for (usize n = 3; n >= 1; --n) {
			auto opt = state.PeekSome(n);
			if (!opt) return {};
			std::string_view view = *opt;

#ifdef LUNI_DEBUG_INFO
		fmt::print("[Debug][Lexer.Oper] Matching length {}, input: '{}'\n", n, view);
#endif // #ifdef LUNI_DEBUG_INFO

			if (operators.find(view) != operators.end()) {
#ifdef LUNI_DEBUG_INFO
		fmt::print("[Debug][Lexer.Oper] Found matching operator '{}' with length {}\n", view, n);
#endif // #ifdef LUNI_DEBUG_INFO

				auto pos = TokenPos::CurrentPos(state);
				state.Advance(n);
				return Token{std::string{view}, pos, TokenType::OPERATOR};
			}

#ifdef LUNI_DEBUG_INFO
		fmt::print("[Debug][Lexer.Oper] No matching operator found with length {}\n", n);
#endif // #ifdef LUNI_DEBUG_INFO
		}

#ifdef LUNI_DEBUG_INFO
		fmt::print("[Debug][Lexer.Oper] No matching operator found with either lengths, aborting\n");
#endif // #ifdef LUNI_DEBUG_INFO
		return {};
	}

	auto TryLexSimpleString(LexingState& state) -> std::string {
		std::string buf;
		while (true) {
			auto opt = state.Take();
			if (!opt) break;
			auto c = *opt;

#ifdef LUNI_DEBUG_INFO
		fmt::print("[Debug][Lexer.Str] Fetched char '{}'\n", c);
#endif // #ifdef LUNI_DEBUG_INFO

			if (c == '"') {
#ifdef LUNI_DEBUG_INFO
		fmt::print("[Debug][Lexer.Str] Found string literal ending, result: '{}'\n", buf);
#endif // #ifdef LUNI_DEBUG_INFO
				break;
			}
			
			buf += c;
#ifdef LUNI_DEBUG_INFO
		fmt::print("[Debug][Lexer.Str] Current buffer: '{}'\n", buf);
#endif // #ifdef LUNI_DEBUG_INFO
		}

		return buf;
	}
	auto TryLexMultilineString(LexingState& state) -> std::string {
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
		if (state.Peek().value_or(' ') == '"') {
#ifdef LUNI_DEBUG_INFO
		fmt::print("[Debug][Lexer.Str] Found string literal beginning\n");
#endif // #ifdef LUNI_DEBUG_INFO

			state.Advance();
			auto pos = TokenPos::CurrentPos(state);
			return Token{TryLexSimpleString(state), pos, TokenType::STRING_LITERAL};
		}

		if (state.PeekSome(2).value_or("") == "[[") {
#ifdef LUNI_DEBUG_INFO
		fmt::print("[Debug][Lexer.Str] Found multiline string literal beginning\n");
#endif // #ifdef LUNI_DEBUG_INFO

			state.Advance(2);
			auto pos = TokenPos::CurrentPos(state);
			return Token{TryLexMultilineString(state), pos, TokenType::MULTILINE_STRING_LITERAL};
		}

#ifdef LUNI_DEBUG_INFO
		fmt::print("[Debug][Lexer.Str] No string literal beginning found\n");
#endif // #ifdef LUNI_DEBUG_INFO
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
		// Keep consuming input until we see a multiline comment end
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
	auto TryLexComments(LexingState& state) -> std::optional<TokenPos> {
		auto first2Opt = state.PeekFull(2);
		if (!first2Opt) return {};
		auto first2 = *first2Opt;

		// block comment marks also starts with --
		if (first2 != lineComment) {
#ifdef LUNI_DEBUG_INFO
		fmt::print("[Debug][Lexer.Comment] No comment beginning found\n");
#endif // #ifdef LUNI_DEBUG_INFO
			return {};
		}

		// From this point we know we have a comment
		auto pos = TokenPos::CurrentPos(state);
		state.Advance(2);

		auto next2 = state.PeekFull(2, 2);
		if (next2 && *next2 == "[[") {
#ifdef LUNI_DEBUG_INFO
		fmt::print("[Debug][Lexer.Comment] Found multiline comment beginning\n");
#endif // #ifdef LUNI_DEBUG_INFO

			state.Advance(2);
			TryLexMultilineComment(state);
		} else {
#ifdef LUNI_DEBUG_INFO
		fmt::print("[Debug][Lexer.Comment] Found line comment beginning\n");
#endif // #ifdef LUNI_DEBUG_INFO

			TryLexLineComment(state);
		}
		return pos;
	}
}

auto LuNI::DoLexing(
	argparse::ArgumentParser* args,
	const std::string& text
) -> LexingState {
	auto verbose = (*args)["--verbose-lexing"] == true;

	LexingState state{text};
	while (state.HasNext()) {
		auto iden = TryLexIdentifier(state);
		if (iden) {
			auto it = keywords.find(iden->text);
			if (it != keywords.end()) iden->type = TokenType::KEYWORD;

			if (verbose) {
				fmt::print("Generated {} token '{}'\n", iden->type, iden->text);
				fmt::print("\tstarting at {}\n", iden->pos);
			}
			state.AddToken(std::move(*iden));
			continue;
		}

		auto str = TryLexString(state);
		if (str) {
			if (verbose) {
				fmt::print("Generated string literal token '{}'\n", str->text);
				fmt::print("\tstarting at {}\n", str->pos);
			}
			state.AddToken(std::move(*str));
			continue;
		}

		auto commentPos = TryLexComments(state);
		if (commentPos) {
			if (verbose) {
				fmt::print("Discarded comments starting at {}\n", *commentPos);
			}
			continue;
		}

		auto oper = TryLexOperator(state);
		if (oper) {
			if (verbose) {
				fmt::print("Generated operator token '{}'\n", oper->text);
				fmt::print("\tstarting at {}\n", oper->pos);
			}
			state.AddToken(std::move(*oper));
			continue;
		}

		if (std::isspace(state.Peek().value_or('\0'))) {
			if (verbose) {
				fmt::print("Discarded whitespace at {}\n", TokenPos::CurrentPos(state));
			}
			state.Advance();
			continue;
		}
	}
	return state;
}

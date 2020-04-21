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

Lexer::Lexer(argparse::ArgumentParser* program) noexcept
	: program{ program } {
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

	auto TryLexIdentifier(LexingState& state) -> std::optional<Token> {
		auto beginning = state.Peek().value_or(' ');
		if (!std::regex_match(std::string{beginning}, identifierBegin)) return {};
		// We know this must be an identifier (or keyword) at this point
		auto pos = TokenPos::CurrentPos(state);

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
			if (operators.find(view) != operators.end()) {
				auto pos = TokenPos::CurrentPos(state);
				state.Advance(n);
				return Token{std::string{view}, pos, TokenType::OPERATOR};
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
			state.Advance();
			auto pos = TokenPos::CurrentPos(state);
			return Token{TryLexSimpleString(state), pos, TokenType::STRING_LITERAL};
		}
		if (state.PeekSome(2).value_or("") == "[[") {
			state.Advance(2);
			auto pos = TokenPos::CurrentPos(state);
			return Token{TryLexMultilineString(state), pos, TokenType::MULTILINE_STRING_LITERAL};
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
		auto first2 = state.PeekFull(2).value();
		// block comment marks also starts with --
		if (first2 != lineComment) return {};
		// From this point we know we have a comment
		auto pos = TokenPos::CurrentPos(state);
		state.Advance(2);

		auto next2 = state.PeekFull(2, 2);
		if (next2 && *next2 == "[[") {
			state.Advance(2);
			TryLexMultilineComment(state);
		} else {
			TryLexLineComment(state);
		}
		return pos;
	}
}
auto Lexer::DoLexing(std::string text) -> LexingState {
	auto verbose = (*program)["--verbose-lexing"] == true;

	LexingState state{std::move(text)};
	while (state.HasNext()) {
		auto iden = TryLexIdentifier(state);
	if (iden) {
			auto it = keywords.find(iden->text);
			if (it != keywords.end()) iden->type = TokenType::KEYWORD;

			if (verbose) {
				fmt::print("Generated {} token \"{}\";\n\tstarting at {}",
							iden->type == TokenType::KEYWORD ? "keyword" : "identifier",
							iden->text,
							iden->pos);
			}
			state.AddToken(std::move(*iden));
			continue;
		}

		auto str = TryLexString(state);
		if (str) {
			if (verbose) {
				fmt::print("Generated string literal token \"{}\";\n\tstarting at {}", str->text, str->pos);
			}
			state.AddToken(std::move(*str));
			continue;
		}

		auto commentPos = TryLexComments(state);
		if (commentPos) {
			if (verbose) {
				fmt::print("Discarded comments starting at {}", *commentPos);
			}
			continue;
		}

		auto oper = TryLexOperator(state);
		if (oper) {
			if (verbose) {
				fmt::print("Generated operator token \"{}\";\n\tstarting at {}", oper->text, oper->pos);
			}
			state.AddToken(std::move(*oper));
			continue;
		}

		if (std::isspace(state.Peek().value_or('\0'))) {
			// No verbose output for this guy because no
			state.Advance();
			continue;
		}
	}
	return state;
}

template <>
struct fmt::formatter<LuNI::TokenType> {
	constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

	template <typename FormatContext>
	auto format(const LuNI::TokenType& type, FormatContext& ctx) {
		using LuNI::TokenType;
		switch (type) {
			case TokenType::IDENTIFIER: return "identifier";
			case TokenType::KEYWORD: return "keyword";
			case TokenType::OPERATOR: return "operator";
			case TokenType::STRING_LITERAL: return "string literal";
			case TokenType::MULTILINE_STRING_LITERAL: return "multiline string literal";
		}
		throw std::runtime_error("Invalid token type");
	}
};
std::ostream& operator<<(std::ostream& out, const TokenType& type) {
	out << fmt::format("{}", type);
	return out;
}

template <>
struct fmt::formatter<LuNI::TokenPos> {
	constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

	template <typename FormatContext>
	auto format(const LuNI::TokenPos& pos, FormatContext& ctx) {
		return format_to(ctx.out(), "{}:{}", pos.line, pos.column);
	}
};
std::ostream& operator<<(std::ostream& out, const TokenPos& pos) {
	out << fmt::format("{}:{}", pos.line, pos.column);
	return out;
}

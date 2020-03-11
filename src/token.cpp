#include <functional>
#include <optional>
#include <regex>
#include <unordered_set>
#include "token.hpp"

namespace LuNI {

auto TokenStream::PushToken(Token token) -> void {
	tokens.push_back(std::move(token));
}
auto TokenStream::PollToken() -> Token {
	auto t = tokens.front();
	tokens.pop_front();
	return t;
}
auto TokenStream::Front() const -> const Token& { return tokens.front(); }
auto TokenStream::View() const -> TokenStreamView { return {*this, 0}; }

TokenStreamView::TokenStreamView(const TokenStream& dataIn, usize startIn)
		: data{dataIn}, ptr{startIn}, genSnapshot{data.generation} {
}
auto TokenStreamView::Next() -> const Token& {
	auto& result = this->Front();
	++ptr;
	return result;
}
auto TokenStreamView::Front() const -> const Token& {
	if (genSnapshot != data.generation)
		throw std::runtime_error("Illegal state: this TokenStreamView references an older TokenStream than the current one");
	else
		return data.tokens.at(ptr);
}

static const std::unordered_set<std::string> keywords{
	"and", "break",		 "do",		 "else", "elseif", "end",		"false",
	"for", "function", "if",		 "in",	 "local",  "nil",		"not",
	"or",  "repeat",	 "return", "then", "true",	 "until", "while",
};
static const std::regex identifierMatcher("[a-zA-Z_][0-9a-zA-Z_]*");
static const std::unordered_set<std::string> operators{
	"+", "-", "*", "/", "%", "^", "#", "==", "~=", "<=", ">=", "<",  ">",
	"=", "(", ")", "{", "}", "[", "]", ";",  ":",  ",",  ".",  "..", "...",
};
static const std::unordered_set<char> operatorBeginnings = std::invoke([]() {
	auto result = std::unordered_set<char>{};
	for (const auto& oper : operators) {
		result.insert(oper[0]);
	}
	return result;
});
static const std::string lineComment = "--";
static const std::string blockCommentBeg = "--[";
static const std::string blockCommentEnd = "--]";
static const std::string multilineStringBeg = "[[";

class PSBase {
public:
	// Fuck C++ there's no order bootstraping
	// Forward declare the function to be used in PSBase and itself references other things
	static auto CreateFrom(char c, CharInfo info) -> std::optional<PSBase>;

	virtual auto Feed(TokenStream& stream, char c, CharInfo info) -> std::optional<PSBase> {
		return PSBase::CreateFrom(c, info);
	}
};

class PSIdentifier : public PSBase {
private:
	CharInfo tokenInfo;
	std::string builder;

public:
	PSIdentifier(char c, CharInfo info) : tokenInfo{info}, builder{c} {}

	auto Feed(TokenStream& stream, char c, CharInfo info) -> std::optional<PSBase> override {
		if (std::regex_match(std::string{c}, identifierMatcher)) {
			builder += c;
			if (keywords.find(builder) != keywords.end()) {
				stream.PushToken(Token{std::move(builder)});
			}
			return {};
		} else {
			return PSBase::CreateFrom(c, info);
		}
	}
};

// Discarding parsing states
// all inputs should be discarded until comment ends
class PSComment : public PSBase {
public:
	auto Feed(TokenStream& stream, char c, CharInfo info) -> std::optional<PSBase> override {
		if (c == '\n')
			return PSBase{};
		else
			return {};
	}
};
class PSBlockComment : public PSBase {
private:
	u32 stage;

public:
	auto Feed(TokenStream& stream, char c, CharInfo info) -> std::optional<PSBase> override {
		switch (stage) {
			case 0: {
				if (c == '-')
					stage = 0;
				return {};
			}
			case 1: {
				if (c == '-')
					++stage;
				else
					stage = 0;
				return {};
			}
			case 2: {
				if (c == ']')
					return PSBase{};
				else
					stage = 0;
				return {};
			}
			default:
				return {};
		}
	}
};

class PSString : public PSBase {
private:
	CharInfo tokenInfo;
	std::string builder;

public:
	PSString(CharInfo info) : tokenInfo{info} {}

	auto Feed(TokenStream& stream, char c, CharInfo info) -> std::optional<PSBase> override {
		// Technically an unterminated string is a syntax error
		// Error generation will be postponed until AST gen
		if (c == '"' || c == '\n') {
			stream.PushToken(Token{std::move(builder)});
			return PSBase{};
		} else {
			builder += c;
			return {};
		}
	}
};
class PSMultilineString : public PSBase {
private:
	CharInfo tokenInfo;
	u32 endingStage = 0;
	std::string builder;

public:
	PSMultilineString(CharInfo info) : tokenInfo{info} {}
	
	auto Feed(TokenStream& stream, char c, CharInfo info) -> std::optional<PSBase> override {
		if(c == ']') {
			++endingStage;
			if(endingStage == 2) {
				stream.PushToken(Token{std::move(builder)});
				return {};
			}
		} else {
		 builder += c;
		}
		return {};
	}
};

class PSOperator : public PSBase {
private:
	CharInfo tokenInfo;
	std::string builder;

public:
	PSOperator(char c, CharInfo info) : tokenInfo{info}, builder{c} {}

	auto Feed(TokenStream& stream, char c, CharInfo info) -> std::optional<PSBase> override {
		if (operatorBeginnings.find(c) != operatorBeginnings.end()) {
			builder += c;
			if (builder == lineComment)
				return PSComment{};
			else if (builder == blockCommentBeg)
				return PSBlockComment{};
			else if (builder == multilineStringBeg)
				return PSMultilineString{tokenInfo};
			else
				return {};
		} else {
			stream.PushToken(Token{std::move(builder)});
			return PSBase::CreateFrom(c, info);
		}
	}
};

auto PSBase::CreateFrom(char c, CharInfo info) -> std::optional<PSBase> {
	if (std::regex_match(std::string{c}, identifierMatcher))
		return PSIdentifier{c, info};
	if (operatorBeginnings.find(c) != operatorBeginnings.end())
		return PSOperator{c, info};
	// String literal tokens doesn't include opening and closing quotation marks
	// Multiline string will be derived from PSOperator
	if (c == '"') return PSString{info};
	return {};
}

auto ParseText(TokenStream& stream, std::string& text) -> void {
	auto st = PSBase{};
	auto line = u32{1};
	auto column = u32{1};
	for (char c : text) {
		auto res = st.Feed(stream, c, {line, column});
		if (res.has_value()) st = std::move(*res);

		if(c == '\n') {
			++line;
			column = 0;
		} else {
			++column;
		}
	}
}

}  // namespace LuNI

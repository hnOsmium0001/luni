#include "token.hpp"

#include <regex>
#include <unordered_set>

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
auto TokenStream::View() const -> TokenStreamView { return {this, 0}; }

TokenStreamView::TokenStreamView(const TokenStream* data, usize start)
    : data_{data}, ptr_{start} {
  genSnapshot_ = data->generation;
}
auto TokenStreamView::Next() -> std::optional<const Token*> {
  if (genSnapshot_ != data_->generation)
    return {};
  else
    return &data_->tokens.at(ptr_++);
}

static const std::unordered_set<std::string> keywords{
    "and", "break",    "do",     "else", "elseif", "end",   "false",
    "for", "function", "if",     "in",   "local",  "nil",   "not",
    "or",  "repeat",   "return", "then", "true",   "until", "while",
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

// Fuck C++ there's no order bootstraping
class PSBase;
auto DecideFromNone(char c) -> std::optional<PSBase>;

class PSBase {
 public:
  static auto CreateFrom(char c) -> std::optional<PSBase> {
    static auto ps = PSBase{};
    return ps.Feed(nullptr, c);
  }

  virtual auto Feed(TokenStream* stream, char c) -> std::optional<PSBase> {
    return DecideFromNone(c);
  }
};

class PSIdentifier : public PSBase {
 private:
  std::string builder;

 public:
  PSIdentifier(char c) : builder{c} {}

  auto Feed(TokenStream* stream, char c) -> std::optional<PSBase> override {
    if (std::regex_match(std::string{c}, identifierMatcher)) {
      builder += c;
      if (keywords.find(builder) != keywords.end()) {
        stream->PushToken(Token{std::move(builder)});
      }
      return {};
    } else {
      return PSBase::CreateFrom(c);
    }
  }
};

// Discarding parsing states
// all inputs should be discarded until comment ends
class PSComment : public PSBase {
 public:
  auto Feed(TokenStream* stream, char c) -> std::optional<PSBase> override {
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
  auto Feed(TokenStream* stream, char c) -> std::optional<PSBase> override {
    switch (stage) {
      case 0: {
        if (c == '-')
          ++stage;
        else
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

class PSOperator : public PSBase {
 private:
  std::string builder;

 public:
  PSOperator(char c) : builder{c} {}

  auto Feed(TokenStream* stream, char c) -> std::optional<PSBase> override {
    if (operatorBeginnings.find(c) != operatorBeginnings.end()) {
      builder += c;
      if (builder == lineComment)
        return PSComment{};
      else if (builder == blockCommentBeg)
        return PSBlockComment{};
      else
        return {};
    } else {
      stream->PushToken(Token{std::move(builder)});
      return PSBase::CreateFrom(c);
    }
  }
};

class PSString : public PSBase {
 public:
  std::string builder;

  auto Feed(TokenStream* stream, char c) -> std::optional<PSBase> override {
    if (c == '"') {
      stream->PushToken(Token{std::move(builder)});
      return PSBase{};
    } else {
      builder += c;
      return {};
    }
  }
};

auto DecideFromNone(char c) -> std::optional<PSBase> {
  if (std::regex_match(std::string{c}, identifierMatcher))
    return PSIdentifier{c};
  if (operatorBeginnings.find(c) != operatorBeginnings.end())
    return PSOperator{c};
  // String literal tokens doesn't include opening and closing quotation marks
  if (c == '"') return PSString{};
  return {};
}

auto ParseText(TokenStream* stream, std::string& text) -> void {
  auto st = PSBase{};

  for (char c : text) {
    auto res = st.Feed(stream, c);
    if (res.has_value()) {
      st = std::move(*res);
    }
  }
}

}  // namespace LuNI

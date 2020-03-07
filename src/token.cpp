#include "token.hpp"

#include <regex>
#include <unordered_set>

namespace luni {

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

class PSBase {
 public:
  static auto CreateFrom(char c) -> std::optional<PSBase> {
    static auto ps = PSEmpty{};
    return ps.Feed(nullptr, c);
  }

  virtual auto Feed(TokenStream* stream, char c) -> std::optional<PSBase> {
    throw std::runtime_error("Base method must be overriden");
  }
};

class PSEmpty : public PSBase {
 public:
  auto Feed(TokenStream* stream, char c) -> std::optional<PSBase> override {
    // Character voided
  }
};

static const std::unordered_set<std::string> keywords{
    "and", "break",    "do",     "else", "elseif", "end",   "false",
    "for", "function", "if",     "in",   "local",  "nil",   "not",
    "or",  "repeat",   "return", "then", "true",   "until", "while",
};
static const std::regex identifierMatcher("[a-zA-Z_][0-9a-zA-Z_]*");
class PSIdentifier : public PSBase {
 private:
  std::string builder;

 public:
  auto Feed(TokenStream* stream, char c) -> std::optional<PSBase> override {
    if (std::regex_match(std::string{c}, identifierMatcher)) {
      builder.append(c);
      if (keywords.find(builder) != keywords.end()) {
        stream->PushToken(Token{std::move(builder)});
      }
      return {};
    } else {
      return PSBase::CreateFrom(c);
    }
  }
};

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

class PSOperator : public PSBase {
 private:
  std::string builder;

 public:
  auto Feed(TokenStream* stream, char c) -> std::optional<PSBase> override {
    if (operatorBeginnings.find(c) != operatorBeginnings.end()) {
      builder.append(c);
      return {};
    } else {
      stream->PushToken(Token{std::move(builder)});
      return PSEmpty::CreateFrom(c);
    }
  }
};

static const std::string lineComment = "--";
static const std::string blockCommentBeg = "--[";
static const std::string blockCommentEnd = "--]";
class PSComment : public PSBase {
 public:
  auto Feed(TokenStream* stream, char c) -> std::optional<PSBase> override {
    if (c == '\n')
      return PSEmpty{};
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
          return PSEmpty{};
        else
          stage = 0;
        return {};
      }
      default:
        return {};
    }
  }
};

auto ParseText(TokenStream* stream, std::string& text) -> void {
  PSBase st = PSEmpty{};

  for (char c : text) {
    auto res = st.Feed(stream, c);
    if (res.has_value()) {
      st = std::move(*res);
    }
  }
}

}  // namespace luni

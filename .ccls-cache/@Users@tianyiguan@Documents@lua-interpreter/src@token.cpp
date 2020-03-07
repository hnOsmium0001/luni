#include "token.hpp"
#include <unordered_set>
#include <variant>

namespace luni {

auto TokenStream::PushToken(Token token) -> void {
  tokens.push_back(std::move(token));
}
auto TokenStream::PollToken() -> Token {
  auto t = tokens.front();
  tokens.pop_front();
  return t;
}
auto TokenStream::Front() -> const Token& const { return tokens.front(); }
auto TokenStream::View() -> TokenStreamView const { return {this, 0}; }

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
  PSBase() = default;
  PSBase(const PSBase&) = delete;
  PSBase(PSBase&&) = default;
  virtual auto Feed(TokenStream* stream, char32_t c) -> std::optional<PSBase> {
    throw std::runtime_error("Base method must be overriden");
  }
};

class PSEmpty : public PSBase {
 public:
  auto Feed(TokenStream* stream, char32_t c) -> std::optional<PSBase> override {
    // Character voided
  }
};

class PSIdentifier : public PSBase {
 private:
  std::string builder;

 public:
  auto Feed(TokenStream* stream, char32_t c) -> std::optional<PSBase> override {
    // TODO
  }
};

static const std::unordered_set<std::string> keywords{
    "and", "break",    "do",     "else", "elseif", "end",   "false",
    "for", "function", "if",     "in",   "local",  "nil",   "not",
    "or",  "repeat",   "return", "then", "true",   "until", "while",
};
class PSKeyword : public PSBase {
 private:
  std::string builder;

 public:
  auto Feed(TokenStream* stream, char32_t c) -> std::optional<PSBase> override {
    // TODO
  }
};

static const std::unordered_set<std::string> operators{
    "+", "-", "*", "/", "%", "^", "#", "==", "~=", "<=", ">=", "<",  ">",
    "=", "(", ")", "{", "}", "[", "]", ";",  ":",  ",",  ".",  "..", "...",
};
static const std::unordered_set<char32_t> operatorBeginnings =
    std::invoke([]() {
      auto result = std::unordered_set<char32_t>{};
      for (const auto& oper : operators) {
        // TODO utf8 proper way
        result.insert(*oper.begin());
      }
      return result;
    });

class PSOperator : public PSBase {
 private:
  std::string builder;

 public:
  auto Feed(TokenStream* stream, char32_t c) -> std::optional<PSBase> override {
    // TODO
  }
};

static const std::string lineComment = "--";
static const std::string blockCommentBeg = "--[";
static const std::string blockCommentEnd = "--]";
class PSComment : public PSBase {
 public:
  auto Feed(TokenStream* stream, char32_t c) -> std::optional<PSBase> override {
    return c == '\n' ? PSEmpty{} : std::nullopt;
  }
};
class PSBlockComment : public PSBase {
 private:
  u32 stage;

 public:
  auto Feed(TokenStream* stream, char32_t c) -> std::optional<PSBase> override {
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
      case 2:
        if (c == ']')
          return PSEmpty{};
        else
          stage = 0;
        return {};
      default:
        return {};
    }
  }
};

using ParseState = std::variant<PSEmpty, PSIdentifier, PSKeyword, PSOperator,
                                PSComment, PSBlockComment>;

auto ParseText(TokenStream* stream, std::string& text) -> void {
  ParseState st = PSEmpty{};

  // TODO utf8 proper way
  for (char32_t c : text) {
    std::visit(
        [&st, stream, c](auto&& v) {
          std::optional<PSBase> res = v.Feed(stream, c);
          if (res.has_value()) {
            st = *res;
          }
        },
        st);
  }
}

}  // namespace luni

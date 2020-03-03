#include <ctime>
#include <string>
#include <optional>
#include <variant>
#include "token.hpp"

namespace luni {

TokenStreamView TokenStream::View() const {
  return {this, 0};
}

TokenStreamView::TokenStreamView(const TokenStream* data, usize start) 
  : data_{ data }, ptr_{ start } {
  genSnapshot_ = data->generation;
}

std::optional<const Token&> TokenStreamView::Next() {
  if (genSnapshot_ != data_->generation) return {};
  else return data_->tokens.at(ptr_++);
}

static const std::string keywords[]{
  "and",
  "break",
  "do",
  "else",
  "elseif",
  "end",
  "false",
  "for",
  "function",
  "if",
  "in",
  "local",
  "nil",
  "not",
  "or",
  "repeat",
  "return",
  "then",
  "true",
  "until",
  "while",
};
static const std::string operators[]{
  "+",
  "-",
  "*",
  "/",
  "%",
  "^",
  "#",
  "==",
  "~=",
  "<=",
  ">=",
  "<",
  ">",
  "=",
  "(",
  ")",
  "{",
  "}",
  "[",
  "]",
  ";",
  ":",
  ",",
  ".",
  "..",
  "...",
};
static const std::string lineComment = "--";
static const std::string blockCommentBeg = "--[";
static const std::string blockCommentEnd = "--]";

enum class ParsingMode {
  UNKNOWN,
  IDENTIFIER,
  KEYWORD,
  OPERATOR,
};
class BaseParseState {
public:
  std::string builder;
  virtual bool IsComplete();
  virtual void Feed(char32_t c);
};
class ParseState_Identifier : BaseParseState {
};
class ParseState_Keyword : BaseParseState {

};
class ParseState_Operator : BaseParseState {

};
using ParseState = std::variant<std::monostate, ParseState_Identifier, ParseState_Keyword, ParseState_Operator>;

void ParseText(TokenStream *stream, std::string &text) {
  auto md = ParsingMode::UNKNOWN;
  auto st = ParseState{std::monostate};

  for(char32_t c : text) {

  }
}

} // namespace luni

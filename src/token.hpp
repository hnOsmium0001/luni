#include <cstddef>
#include <string>
#include <deque>
#include <optional>
#include "aliases.hpp"

namespace luni {

// Existance declarations
class Token;
class TokenStream;
class TokenStreamView;

// Actual desclarations.
enum class TokenType {
  KEYWORD,
  IDENTIFIER,
  LITERAL,
};

class Token {
public:
  std::string str;
  TokenType type;
};

class TokenStream {
private:
  std::deque<Token> tokens;
  u32 generation;

public:
  void PushToken(Token token);
  Token PollToken();
  const Token& Top();
  TokenStreamView View() const;

  friend class TokenStreamView;
};

class TokenStreamView {
public:
  const TokenStream* data_;
  usize ptr_;
  u32 genSnapshot_;

  TokenStreamView(const TokenStream* data, usize start);

  std::optional<const Token&> Next();
};

void ParseText(TokenStream* stream, std::string& text);

} // namespace luni
#include <cstddef>
#include <deque>
#include <optional>
#include <string>
#include "aliases.hpp"

namespace luni {

// Existance declarations
class Token;
class TokenStream;
class TokenStreamView;

class Token {
 public:
  std::string str;
};

class TokenStream {
 private:
  std::deque<Token> tokens;
  u32 generation;

 public:
  auto PushToken(Token token) -> void;
  auto PollToken() -> Token;
  auto Front() -> const Token& const;
  auto View() -> TokenStreamView const;

  friend class TokenStreamView;
};

class TokenStreamView {
 public:
  const TokenStream* data_;
  usize ptr_;
  u32 genSnapshot_;

  TokenStreamView(const TokenStream* data, usize start);

  auto Next() -> std::optional<const Token*>;
};

auto ParseText(TokenStream* stream, std::string& text) -> void;

}  // namespace luni
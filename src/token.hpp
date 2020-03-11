#include <cstddef>
#include <deque>
#include <string>
#include "aliases.hpp"

namespace LuNI {

struct CharInfo {
	u32 line;
	u32 column;
};

class Token {
 public:
  std::string str;
	CharInfo info; // Information about the beginning character of this token
};

class TokenStream;
class TokenStreamView;

class TokenStream {
 private:
  std::deque<Token> tokens;
  u32 generation;

 public:
  auto PushToken(Token token) -> void;
  auto PollToken() -> Token;
  auto Front() const -> const Token&;
  auto View() const -> TokenStreamView;

  friend class TokenStreamView;
};

class TokenStreamView {
 public:
  const TokenStream& data;
  usize ptr;
  const u32 genSnapshot;

  TokenStreamView(const TokenStream& dataIn, usize startIn);

  auto Next() -> const Token&;
	auto Front() const -> const Token&;
};

auto ParseText(TokenStream& stream, std::string& text) -> void;

}  // namespace LuNI

#include <ostream>
#include <string>
#include <vector>
#include <memory>
#include "aliases.hpp"

namespace LuNI {

class ParseError {
private:
  u32 code;
	std::string msg;

public:
	ParseError(u32 codeIn, std::string msgIn) noexcept;
  ParseError(const ParseError&) = delete;
  ParseError& operator=(const ParseError&) = delete;
  ParseError(ParseError&& src) = default;
  ParseError& operator=(ParseError&& src) = default;

	auto WriteTo(std::ostream& out) const -> void;
};
using ParseErrorList = std::vector<ParseError>;

class STNode {
private:
	STNode* parent;
	std::vector<STNode*> children;

public:
	STNode() noexcept;
	STNode(STNode* parentIn, std::vector<STNode*> childrenIn) noexcept;
	STNode(const STNode&) = delete;
	STNode& operator=(const STNode&) = delete;
	STNode(STNode&& src) = default;
	STNode& operator=(STNode&& src) = default;

	auto IsRoot() const -> bool;
	auto IsLeaf() const -> bool;
};

class SyntaxTree {
private:
	std::vector<std::unique_ptr<STNode>> pool;
	STNode* root;

public:
	SyntaxTree() noexcept;
	SyntaxTree(const SyntaxTree&) = delete;
	SyntaxTree& operator=(const SyntaxTree&) = delete;
	SyntaxTree(SyntaxTree&& src) = default;
	SyntaxTree& operator=(SyntaxTree&& src) = default;

	auto CreateNode() -> STNode*;
};

} // namespace LuNI

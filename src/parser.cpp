#include "parser.hpp"

namespace LuNI {

ParseError::ParseError(u32 codeIn, std::string msgIn) noexcept
	: code{codeIn}, msg{msgIn} {
}
auto ParseError::WriteTo(std::ostream& out) const -> void {
	out << "EParse" << code << " - " << msg << "\n";
}

STNode::STNode() noexcept : parent{nullptr}, children{} {}
STNode::STNode(STNode* parentIn, std::vector<STNode*> childrenIn) noexcept
    : parent{parentIn}, children{std::move(childrenIn)} {}
auto STNode::IsRoot() const -> bool { return parent == nullptr; }
auto STNode::IsLeaf() const -> bool { return children.size() == 0; }

SyntaxTree::SyntaxTree() noexcept {}
auto SyntaxTree::CreateNode() -> STNode* {
  this->pool.push_back(std::make_unique<STNode>());
  return this->pool.back().get();
}

} // namespace LuNI

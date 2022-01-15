#include "AstNode.hpp"

#include "Util.hpp"

#include <cassert>

using namespace LuNI;

AstNode::AstNode(Kind kind) noexcept
	: kind{ kind } {}

AstScriptNode::AstScriptNode()
	: AstNode(KD_Script) {}

AstNumericLiteralNode::AstNumericLiteralNode()
	: AstNode(KD_NumericLiteral) {
}

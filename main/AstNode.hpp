#pragma once

#include <memory>
#include <span>
#include <string_view>
#include <vector>

namespace LuNI {

class AstNode {
public:
	enum Kind {
		KD_Script, //< 根节点

		KD_NumericLiteral,
		KD_StringLiteral,
		KD_ArrayLiteral,
		KD_MetatableLiteral,
		KD_FunctionDefinition,
		KD_If,
		KD_While,
		KD_Until,
		KD_For,
		KD_LocalVarDef,
		KD_GlobalVarDef, //< 上方节点的任意组合，包括FUNCTION_CALL
		KD_StatementBlock, //< 既可以是表达式也可以是语句

		KD_FunctionCall,

		kKindCount,
	};

public:
	Kind kind;

public:
	AstNode(Kind kind) noexcept;
	AstNode(const AstNode& that) noexcept = delete;
	AstNode& operator=(const AstNode& that) noexcept = delete;
	AstNode(AstNode&& that) noexcept = default;
	AstNode& operator=(AstNode&& that) noexcept = default;

	virtual std::span<AstNode> GetChildren() const = 0;
};

// ========================================
// Misc nodes
// ========================================

class AstScriptNode : public AstNode {
public:
	AstScriptNode();
	// TODO
};

// ========================================
// Literal nodes
// ========================================

class AstNumericLiteralNode : public AstNode {
public:
	double value;

public:
	AstNumericLiteralNode();
};

// ========================================
// Control flow nodes
// ========================================

class AstIfNode : public AstNode {
public:
	std::unique_ptr<AstNode> condition;
	std::unique_ptr<AstNode> ifBody;
	std::unique_ptr<AstNode> elseBody;

public:
	AstIfNode();
};

class AstWhileNode : public AstNode {
public:
	std::unique_ptr<AstNode> condition;
	bool invertCondition = false; // If true, this will the effectively an `until` statement

public:
	AstWhileNode();
};

// ========================================
// Expression nodes
// ========================================

} // namespace LuNI

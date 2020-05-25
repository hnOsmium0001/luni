#include <string>
#include <utility>
#include <variant>
#include <unordered_map>
#include <stack>
#include <tl/expected.hpp>
#include "Interpreter.hpp"

using namespace LuNI;

namespace {
class LuaFunction {
	std::variant<
		std::reference_wrapper<const ASTNode>,
		auto() -> void, // main :: () -> ()
		auto(std::string_view) -> void // print :: (string) -> ()
	> def;
};

class StackFrame {
private:
	std::string_view name;
	std::reference_wrapper<const LuaFunction> owner;
public:
	u32 insCounter = 0;

public:
	StackFrame(std::string_view name, const LuaFunction& node) noexcept
		: name{ std::move(name) }
		, owner{ std::cref(node) } {}

	auto Name() const -> std::string_view { return name; }
	auto Owner() const -> const LuaFunction& { return owner.get(); }
};
}

auto LuNI::RunProgram_WalkAST(
	argparse::ArgumentParser& args,
	const ASTNode& root
) -> void {
	auto callStack = std::stack<StackFrame>{};
	callStack.push(StackFrame::NewMain(root));
	auto functions = std::unordered_map<std::string_view, const ASTNode*>{}; 

	while (!callStack.empty()) {
		auto& stackFrame = callStack.top();
		auto& node = stackFrame.Owner();

		switch (node.type) {
			case ASTType::FUNCTION_DEFINITION:
				auto& paramList = *node.children[1];
				auto& body = *node.children[2];
			case ASTType::SCRIPT: {
				auto& ins = *node.children[stackFrame.insCounter];
				auto& insName = std::get<std::string>(*ins.children[0]);
				++stackFrame.insCounter;

				if (ins.type == ASTType::FUNCTION_DEFINITION) {
					// 创建函数定义
					auto& funcName = std::get<std::string>(*ins.extraData);
					functions.insert({funcName, &ins});
				} else if (insName == "print") {
					// TODO
					auto& text = std::get<std::string>(*paramList.children[0]->extraData);
					fmt::print("{}\n", text);
				} else {
					// 调用函数（ASTType::FUNCTION_CALL -> ASTType::FUNCTION_DEFINITION lookup）
					auto& funcCall = ins; // 重命名
					auto it = functions.find(std::get<std::string>(*funcCall.extraData));
					if (it == functions.end()) return; // TODO

					auto& funcDef = *it->second;
					callStack.push(StackFrame::NewFunc(funcDef));
				}

				if (stackFrame.insCounter >= node.children.size()) {
					break;
				} else {
					continue;
				}
			}
			default: break;
		}

		callStack.pop();
	}
}

auto LuNI::RunProgram(
	argparse::ArgumentParser& args,
	const BytecodeProgram& opcodes
) -> void {
	// TODO
}


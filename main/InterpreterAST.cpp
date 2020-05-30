#include <string>
#include <utility>
#include <variant>
#include <any>
#include <vector>
#include <unordered_map>
#include <stack>
#include <tl/expected.hpp>
#include <hn/Slice.hpp>
#include "Parser.hpp"
#include "Interpreter.hpp"

using namespace LuNI;

namespace {
class LuaVariable {
public:
	std::any val;

	static auto Wrap(std::any val) -> LuaVariable {
		return LuaVariable{std::move(val)};
	}
};

using LuaVariableStore = std::vector<LuaVariable>;

const auto LUA_NIL = LuaVariable{};
const auto LUA_TRUE = LuaVariable{true};
const auto LUA_FALSE = LuaVariable{false};

class SystemFunction {
public:
	using P = hn::ConstSlice<LuaVariableStore>&;
	using ErasedFunction = auto(*)(P params) -> LuaVariable;

private:
	ErasedFunction ptr;

public:
	SystemFunction(ErasedFunction ptr) noexcept : ptr{ ptr } {}
	operator ErasedFunction() const { return ptr; }
};

namespace SystemImpl {
	auto Print(SystemFunction::P params) -> LuaVariable {
		auto& text = std::any_cast<const std::string&>(params[0].val);
		return LUA_NIL;
	}

	auto Sqrt(SystemFunction::P& params) -> LuaVariable {
		auto in = std::any_cast<float>(params[0].val);
		return LuaVariable{static_cast<float>(std::sqrt(in))};
	}

	auto Sin(SystemFunction::P& params) -> LuaVariable {
		auto in = std::any_cast<float>(params[0].val);
		return LuaVariable{static_cast<float>(std::sin(in))};
	}

	auto Cos(SystemFunction::P& params) -> LuaVariable {
		auto in = std::any_cast<float>(params[0].val);
		return LuaVariable{static_cast<float>(std::cos(in))};
	}

	auto Tan(SystemFunction::P& params) -> LuaVariable {
		auto in = std::any_cast<float>(params[0].val);
		return LuaVariable{static_cast<float>(std::tan(in))};
	}
}

class LuaFunctionDef {
public:
	struct Source {
		std::reference_wrapper<const ASTNode> impl;
	};

	using NodeFunction = std::reference_wrapper<const ASTNode>;
	using Impl = std::variant<NodeFunction, SystemFunction>;
	using YieldResult = std::variant<
		std::reference_wrapper<const ASTNode>, // 函数调用
		Source, // 函数定义，喂给StackFrame
		LuaVariable
    >;

	Impl impl;
	u32 paramsCount;

	LuaFunctionDef(Impl impl, u32 paramsCount) noexcept
		: impl{ std::move(impl) }
		, paramsCount{ std::move(paramsCount) } {}

	// 注意LuaFunction是无状态的函数定义，LuaFunction的“实例”就是Interpreter::StackFrame
	auto Invoke(
		hn::ConstSlice<LuaVariableStore>& params,
		hn::ConstSlice<LuaVariableStore>& locals,
		u32* insCounter
	) const -> YieldResult {
		if (impl.index() != 0) {
			return std::get<SystemFunction>(impl)(params);
		}

		while (true) {
			auto& node = std::get<NodeFunction>(impl).get();
			if (*insCounter >= node.children.size()) {
				return LUA_NIL;
			}

			auto& childNode = *node.children[*insCounter];
			switch (childNode.type) {
				case ASTType::FUNCTION_CALL: {
					// TODO
					return childNode;
				}
				// TODO
				default: throw std::runtime_error(fmt::format("Unsupported type {} when executing function body", childNode.type));
			}

			++*insCounter;
		}
		return LUA_NIL;
	}
};

struct StackFrame {
	std::string_view name;
	std::reference_wrapper<const LuaFunctionDef> source;

	LuaVariableStore vars;
	hn::ConstSlice<LuaVariableStore> params;
	hn::ConstSlice<LuaVariableStore> locals;
	u32 insCounter = 0;

	StackFrame(std::string_view name, const LuaFunctionDef& def) noexcept
		: name{ std::move(name) }
		, source{ std::cref(def) }
		, vars{def.paramsCount} // Reserve enough space for the parameters
		, params{ hn::SliceOf(std::as_const(vars), 0, def.paramsCount) }
		, locals{ hn::SliceOf(std::as_const(vars), def.paramsCount, vars.size()) } {}

	StackFrame(const LuaFunctionDef& def, u32 paramsCount) noexcept
		: name{ std::get<std::string>(
			*std::get<LuaFunctionDef::NodeFunction>(def.impl).get().extraData
		)}
		, source{ std::cref(def) }
		, vars{paramsCount} // Reserve enough space for the parameters
		, params{ hn::SliceOf(std::as_const(vars), 0, def.paramsCount) }
		, locals{ hn::SliceOf(std::as_const(vars), def.paramsCount, vars.size()) } {}
};

class Interpreter {
private:
	std::stack<StackFrame> callStack;
	std::unordered_map<std::string_view, LuaFunctionDef> functionDefs;
	LuaFunctionDef main;
	bool verbose;

public:
	Interpreter(argparse::ArgumentParser& args, const ASTNode& root)
		: functionDefs{
			{ "print", LuaFunctionDef{SystemImpl::Print, 1} },
			{ "sqrt", LuaFunctionDef{SystemImpl::Sqrt, 1} },
			{ "sin", LuaFunctionDef{SystemImpl::Sin, 1} },
			{ "cos", LuaFunctionDef{SystemImpl::Cos, 1} },
			{ "tan", LuaFunctionDef{SystemImpl::Tan, 1} }
		}
		, main{ LuaFunctionDef{root, 0} }
		, verbose{ args["--verbose-execution"] == true }
	{
		callStack.push(StackFrame{"<main>", main});
	}

	auto Run() -> tl::expected<u32, RuntimeError> {
		while (!callStack.empty()) {
			auto& stackFrame = callStack.top();
			auto& func = stackFrame.source.get();

			bool finished = false;
			std::visit(
				Overloaded {
					[&](std::reference_wrapper<const ASTNode> nextFunc) {
						// TODO locaj func
						PushFuncCall(nextFunc.get());
						finished = false;
				    },
					[&](LuaFunctionDef::Source&& src) {
						if (&func != &this->main) {
							stackFrame.vars.push_back(LuaVariable::Wrap(std::move(src)));
						} else {
							DefineFunction(src.impl);
						}
						finished = false;
					},
					[&](LuaVariable&& ret) {
						ReturnFromFuncCall(std::move(ret));
						finished = true;
					}
				},
				func.Invoke(stackFrame.params, stackFrame.locals, &stackFrame.insCounter)
			);

			if (finished) callStack.pop();
		}
		return 0;
	}

private:
	auto DefineFunction(const ASTNode& funcDefNode) -> void {
		auto& nameNode = *funcDefNode.children[0];
		auto& funcName = std::get<std::string>(*nameNode.extraData);

		auto& paramsNode = *funcDefNode.children[1];
		auto paramsCount = static_cast<u32>(paramsNode.children.size());

		functionDefs.insert({funcName, LuaFunctionDef{funcDefNode, paramsCount}});
	}

	auto PushFuncCall(const ASTNode& callerNode) -> void {
		auto& calleeName = std::get<std::string>(*callerNode.children[0]->extraData);
		auto it = functionDefs.find(calleeName);
		if (it == functionDefs.end()) return;
		auto& func = it->second;

		callStack.push(StackFrame{calleeName, func});
	}

	auto ReturnFromFuncCall(LuaVariable ret) -> void {

	}
};
}

auto LuNI::RunProgram_WalkAST(
	argparse::ArgumentParser& args,
	const ASTNode& root
) -> void {
	auto interpreter = Interpreter{args, root};
	interpreter.Run();
}


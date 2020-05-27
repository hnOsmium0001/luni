#include <string>
#include <utility>
#include <variant>
#include <any>
#include <vector>
#include <unordered_map>
#include <stack>
#include <tl/expected.hpp>
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

const auto LUA_NIL = LuaVariable{};
const auto LUA_TRUE = LuaVariable{true};
const auto LUA_FALSE = LuaVariable{false};

using LuaVariableStore = std::vector<LuaVariable>;
using LuaParameters = ConstSlice<LuaVariableStore>;
using LuaLocals = ConstSlice<LuaVariableStore>;

class SystemFunction {
private:
	using ErasedFunction = auto(*)(LuaParameters& params) -> LuaVariable;

	ErasedFunction ptr;

public:
	SystemFunction(ErasedFunction ptr) noexcept : ptr{ ptr } {}
	operator ErasedFunction() const { return ptr; }
};

namespace SystemImpl {
	auto Print(LuaParameters& params) -> LuaVariable {
		auto& text = std::any_cast<const std::string&>(params[0].val);
		return LUA_NIL;
	}

	auto Sqrt(LuaParameters& params) -> LuaVariable {
		auto in = std::any_cast<float>(params[0].val);
		return LuaVariable{static_cast<float>(std::sqrt(in))};
	}

	auto Sin(LuaParameters& params) -> LuaVariable {
		auto in = std::any_cast<float>(params[0].val);
		return LuaVariable{static_cast<float>(std::sin(in))};
	}

	auto Cos(LuaParameters& params) -> LuaVariable {
		auto in = std::any_cast<float>(params[0].val);
		return LuaVariable{static_cast<float>(std::cos(in))};
	}

	auto Tan(LuaParameters& params) -> LuaVariable {
		auto in = std::any_cast<float>(params[0].val);
		return LuaVariable{static_cast<float>(std::tan(in))};
	}
}

class LuaFunctionDef {
public:
	using NodeFunction = std::reference_wrapper<const ASTNode>;
	using Impl = std::variant<NodeFunction, SystemFunction>;
	using YieldResult = std::variant<
		std::string_view, // 函数调用
		LuaVariable
    >;

	Impl impl;

	// 注意LuaFunction是无状态的函数定义，LuaFunction的“实例”就是Interpreter::StackFrame
	auto Invoke(
		LuaParameters& params,
		LuaLocals& locals,
		u32* insCounter
	) const -> YieldResult {
		if (impl.index() != 0) {
			return InvokeSystemFunction(params);
		}
		return LuaVariable{}; // TODO
	}

private:
	auto InvokeSystemFunction(const LuaParameters params) const -> LuaVariable {
		return LuaVariable{};
	}
};

struct StackFrame {
	std::string_view name;
	std::reference_wrapper<const LuaFunctionDef> source;

	LuaVariableStore vars;
	LuaParameters params;
	LuaLocals locals;
	u32 insCounter = 0;

	StackFrame(std::string_view name, const LuaFunctionDef& def, u32 paramCount) noexcept
		: name{ std::move(name) }
		, source{ std::cref(def) }
		, params{ LuaParameters::At(vars.begin(), paramCount) }
		, locals{ LuaLocals::Between(vars.begin() + paramCount, vars.end()) } {}

	StackFrame(const LuaFunctionDef& def, u32 paramCount) noexcept
		: name{ std::get<std::string>(
			*std::get<LuaFunctionDef::NodeFunction>(def.impl).get().extraData
		)}
		, source{ std::cref(def) }
		, params{ LuaParameters::At(vars.begin(), paramCount) }
		, locals{ LuaLocals::Between(vars.begin() + paramCount, vars.end()) } {}
};

class ASTWalker {
private:
	std::stack<StackFrame> callStack;
	std::unordered_map<std::string_view, LuaFunctionDef> functionDefs;
	LuaFunctionDef main;
	bool verbose;

public:
	ASTWalker(argparse::ArgumentParser& args, const ASTNode& root)
		: functionDefs{
			{ "print", LuaFunctionDef{SystemImpl::Print} },
			{ "sqrt", LuaFunctionDef{SystemImpl::Sqrt } },
			{ "sin", LuaFunctionDef{SystemImpl::Sin} },
			{ "cos", LuaFunctionDef{SystemImpl::Cos} },
			{ "tan", LuaFunctionDef{SystemImpl::Tan} }
		}
		, main{ LuaFunctionDef{root} }
		, verbose{ args["--verbose-execution"] == true }
	{
		callStack.push(StackFrame{"<main>", std::cref(main), 0});
	}

	auto Run() -> tl::expected<u32, RuntimeError> {
		while (!callStack.empty()) {
			auto& stackFrame = callStack.top();
			auto& func = stackFrame.source.get();

			func.Invoke(stackFrame.params, stackFrame.locals, &stackFrame.insCounter);

			callStack.pop();
		}
		return 0;
	}

private:
	auto DefineFunction(const ASTNode& funcDefNode) -> void {
		auto& nameNode = *funcDefNode.children[0];
		auto& funcName = std::get<std::string>(*nameNode.extraData);
		functionDefs.insert({funcName, LuaFunctionDef{funcDefNode}});
	}

	auto PushFuncCall(const ASTNode& callerNode) -> void {
		auto& calleeName = std::get<std::string>(*callerNode.children[0]->extraData);
		auto it = functionDefs.find(calleeName);
		if (it == functionDefs.end()) return;
		auto& calleeNode = it->second;

		callStack.push(StackFrame{calleeName, calleeNode});
	}
};
}

auto LuNI::RunProgram_WalkAST(
	argparse::ArgumentParser& args,
	const ASTNode& root
) -> void {
	auto interpreter = ASTWalker{args, root};
	interpreter.Run();
}


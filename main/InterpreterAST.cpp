#include <string>
#include <utility>
#include <variant>
#include <any>
#include <vector>
#include <unordered_map>
#include <stack>
#include <stdexcept>
#include <tl/expected.hpp>
#include <tsl/ordered_map.h>
#include "Util.hpp"
#include "Parser.hpp"
#include "Interpreter.hpp"

using namespace LuNI;

namespace {
// TODO gc + 默认引用传参
class LuaValue {
public:
	std::any val;

	static auto Wrap(std::any val) -> LuaValue {
		return LuaValue{std::move(val)};
	}

	static auto Eval(const ASTNode& exprNode) -> LuaValue {
		return {}; // TODO
	}
};

using LuaVariableStore = tsl::ordered_map<std::string_view, LuaValue>;
using LuaVariable = LuaVariableStore::value_type;

const auto LUA_NIL = LuaValue{};
const auto LUA_TRUE = LuaValue{true};
const auto LUA_FALSE = LuaValue{false};

class SystemFunction {
public:
	using P = hn::ConstSlice<LuaVariableStore>&;
	using ErasedFunction = auto(*)(P params) -> LuaValue;

private:
	ErasedFunction ptr;

public:
	SystemFunction(ErasedFunction ptr) noexcept : ptr{ ptr } {}
	operator ErasedFunction() const { return ptr; }
};

namespace SystemImpl {
	usize MAX_PARAMS = 15;
	std::vector<std::string> paramNames = []() {
		std::vector<std::string> result{15};
		for (usize i = 0; i < MAX_PARAMS; ++i) {
			result.push_back(std::to_string(i));
		}
		return result;
	}();

	auto Print(SystemFunction::P params) -> LuaValue {
		auto& text = std::any_cast<const std::string&>(params[0].second.val);
		return LUA_NIL;
	}

	auto Sqrt(SystemFunction::P& params) -> LuaValue {
		auto in = std::any_cast<f32>(params[0].second.val);
		return LuaValue{static_cast<f32>(std::sqrt(in))};
	}

	auto Sin(SystemFunction::P& params) -> LuaValue {
		auto in = std::any_cast<f32>(params[0].second.val);
		return LuaValue{static_cast<f32>(std::sin(in))};
	}

	auto Cos(SystemFunction::P& params) -> LuaValue {
		auto in = std::any_cast<f32>(params[0].second.val);
		return LuaValue{static_cast<f32>(std::cos(in))};
	}

	auto Tan(SystemFunction::P& params) -> LuaValue {
		auto in = std::any_cast<f32>(params[0].second.val);
		return LuaValue{static_cast<f32>(std::tan(in))};
	}
}

class StackFrame;
class LuaFunctionDef {
public:
	using NodeFunction = std::reference_wrapper<const ASTNode>;
	using Impl = std::variant<NodeFunction, SystemFunction>;

	struct FuncCall {
		std::string_view calleeName;
		/// 函数调用表达式所提供的所有参数，不足的将由Interpreter填充成LUA_NIL，而多余的则会被直接扔掉
		std::vector<LuaValue> params;
	};
	using YieldResult = std::variant<FuncCall, LuaValue>;

	Impl impl;
	u32 paramsCount;

	LuaFunctionDef(Impl impl, u32 paramsCount) noexcept
		: impl{ std::move(impl) }
		, paramsCount{ std::move(paramsCount) } {}

	// 注意LuaFunctionDef是无状态的函数定义，LuaFunction的“实例”是Interpreter::StackFrame
	auto Invoke(StackFrame& stackFrame, LuaVariableStore& globalVars) const -> YieldResult;
};

class StackFrame {
public:
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

	StackFrame(const LuaFunctionDef& def) noexcept
		: name{ std::get<std::string>(
			*std::get<LuaFunctionDef::NodeFunction>(def.impl).get().extraData
		)}
		, source{ std::cref(def) }
		, vars{def.paramsCount} // Reserve enough space for the parameters
		, params{ hn::SliceOf(std::as_const(vars), 0, def.paramsCount) }
		, locals{ hn::SliceOf(std::as_const(vars), def.paramsCount, vars.size()) } {}
};

auto LuaFunctionDef::Invoke(StackFrame& stackFrame, LuaVariableStore& globalVars) const -> LuaFunctionDef::YieldResult {
	//using LuaFunctionDef::NodeFunction;
	//using LuaFunctionDef::Impl;

	if (impl.index() != 0) {
		return std::get<SystemFunction>(impl)(stackFrame.params);
	}

	auto& vars = stackFrame.vars;

	while (true) {
		auto& node = std::get<NodeFunction>(impl).get();
		if (stackFrame.insCounter >= node.children.size()) {
			return LUA_NIL;
		}

		auto& childNode = *node.children[stackFrame.insCounter];
		switch (childNode.type) {
			case ASTType::FUNCTION_CALL: {
				auto& calleeName = std::get<std::string>(*childNode.children[0]->extraData);
				// TODO local函数调用
				//auto it = vars.find(calleeName);
				//if (it == vars.end()) return LUA_NIL;
				//auto& func = std::get<LuaFunctionDef>(it->second.val);

				return FuncCall {
					.calleeName = calleeName,
					.params = [&]() {
						auto params = std::vector<LuaValue>{};
						for (auto it = childNode.children.begin() + 1 /* 跳过函数名节点 */; it < childNode.children.end(); ++it) {
							auto& paramNode = *it; // std::unique_ptr<...>
							params.push_back(LuaValue::Eval(*paramNode));
						}
						return params;
					}(),
				};
			}
			case ASTType::FUNCTION_DEFINITION: {
				// TODO
			}
			case ASTType::VARIABLE_DECLARATION:
			case ASTType::LOCAL_VARIABLE_DECLARATION: {
				auto& storage = childNode.type == ASTType::LOCAL_VARIABLE_DECLARATION
					? stackFrame.vars
					: globalVars;

				storage.insert({
					// 变量名
					std::get<std::string>(*childNode.children[0]->extraData),
					// 内部储存的值（LuaValue）
					LuaValue::Eval(*childNode.children[1]),
				});
				continue;
			}
			// TODO
			default: {
				throw std::runtime_error("Illiegal AST node type in a function node!");
			}
		}

		++stackFrame.insCounter;
	}
	return LUA_NIL;
}

class Interpreter {
private:
	std::stack<StackFrame> callStack;
	std::unordered_map<std::string_view, LuaFunctionDef> functionDefs;
	LuaFunctionDef main;
	/// `main`的StackFrame
	StackFrame* global;
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
		global = &callStack.top();
	}

	auto Run() -> tl::expected<u32, RuntimeError> {
		while (!callStack.empty()) {
			auto& stackFrame = callStack.top();
			auto& func = stackFrame.source.get();

			bool finished = std::visit(
				Overloaded {
					[&](LuaFunctionDef::FuncCall&& funcCall) {
						PushFuncCall(std::move(funcCall));
						return false;
				    },
					[&](LuaValue&& ret) {
						ReturnFromFuncCall(std::move(ret));
						return true;
					}
				},
				func.Invoke(stackFrame, global->vars)
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

	auto PushFuncCall(LuaFunctionDef::FuncCall c) -> void {
		auto funcDefOpt = LookupGlobalVariable<LuaFunctionDef>(c.calleeName);
		if (!funcDefOpt) return;
		auto& funcDef = *funcDefOpt;

		auto stackFrame = StackFrame{c.calleeName, funcDef};
		std::visit(
			Overloaded {
				[&](LuaFunctionDef::NodeFunction&& implRef) {
					auto& impl = implRef.get();
					// FUNCTION_DEFINITION的第一个子节点是该函数的名字
					// 第二个子节点才是FUNC_DEF_PARAMETER_LIST
					auto& funcParamDefs = *impl.children[1]; // ASTType::FUNC_DEF_PARAMETER_LIST
					usize i = 0;
					for (auto&& param : c.params) {
						auto& paramDefNode = *funcParamDefs.children[i]; // ASTType::FUNC_DEF_PARAMETER
						auto& paramNameNode = *paramDefNode.children[0]; // ASTType::IDENTIFIER
						auto& paramName = std::get<std::string>(*paramNameNode.extraData); // const std::string&
						stackFrame.vars.insert({
							paramName,
							std::move(param),
						});
						++i;
					}
				},
				[&](SystemFunction&& impl) {
					for (usize i = 0; i < c.params.size(); ++i) {
						stackFrame.vars.insert({
							SystemImpl::paramNames[i],
							std::move(c.params[i]),
						});
					}
				}
			},
			funcDef.impl
		);
		callStack.push(std::move(stackFrame));
	}

	auto ReturnFromFuncCall(LuaValue ret) -> void {
		// TODO
	}

	/// 尝试在全局变量空间里匹配一个拥有`name`和类型`Result`的变量
	/// 如果成功则返回指向该变量的指针
	/// 否则返回nullptr
	template <typename Result>
	auto LookupGlobalVariable(std::string_view name) -> Result* {
		auto it = global->vars.find(name);
		if (it == global->vars.end()) return nullptr;
		
		// std::any_cast有个针对指针的noexcept重载。带指针版本的any_cast会在类型不匹配时
		// 返回空指针而不会抛异常
		std::any* a = &it->second.val;
		return std::any_cast<Result>(a);
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


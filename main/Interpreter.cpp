#include <string>
#include <utility>
#include <variant>
#include <unordered_map>
#include <stack>
#include <tl/expected.hpp>
#include "Interpreter.hpp"

using namespace LuNI;

namespace {
class LuaVariable {
public:
};

template <typename Callable, u32 id = 0>
struct IDFunc;

template <typename Return, typename... Args, u32 id>
struct IDFunc<auto(Args...) -> Return, id> {
	using PtrType = auto(*)(Args...) -> Return;

	PtrType ptr;

	IDFunc(PtrType ptr) noexcept : ptr{ ptr } {}
	operator PtrType() const { return ptr; }
};

class LuaFunction {
public:
	std::variant<
		std::reference_wrapper<const ASTNode>,
		IDFunc<auto(std::string_view) -> void>, // print :: (string) -> ()
		IDFunc<auto(float) -> float, 0>, // sqrt :: (float) -> float
		IDFunc<auto(float) -> float, 1>, // sin :: (float) -> float
		IDFunc<auto(float) -> float, 2>, // cos :: (float) -> float
		IDFunc<auto(float) -> float, 3> // tan :: (float) -> float
	> def;

public:
	auto Invoke(std::vector<LuaVariable> params) -> LuaVariable {
		return LuaVariable{}; // TODO
	}
};

class StackFrame {
private:
	std::string_view name;
	std::reference_wrapper<const LuaFunction> source;
public:
	u32 insCounter = 0;

public:
	StackFrame(std::string_view name, const LuaFunction& node) noexcept
		: name{ std::move(name) }
		, source{ std::cref(node) } {}

	auto Name() const -> std::string_view { return name; }
	auto Source() const -> const LuaFunction& { return source.get(); }
};

class ASTWalker {
private:
	std::stack<StackFrame> callStack;
	std::unordered_map<std::string_view, LuaFunction> functionDefs;
	LuaFunction main;
	bool verbose;

public:
	ASTWalker(argparse::ArgumentParser& args, const ASTNode& root)
		: functionDefs{
			{ "print", LuaFunction{ASTWalker::Print} },
			{ "sqrt", LuaFunction{IDFunc<auto(float) -> float, 0>{ASTWalker::Sqrt}} },
			{ "sin", LuaFunction{IDFunc<auto(float) -> float, 1>{ASTWalker::Sin}} },
			{ "cos", LuaFunction{IDFunc<auto(float) -> float, 2>{ASTWalker::Cos}} },
			{ "tan", LuaFunction{IDFunc<auto(float) -> float, 3>{ASTWalker::Tan}} }
		}
		, main{ LuaFunction{root} }
		, verbose{ args["--verbose-execution"] == true }
	{
		callStack.push(StackFrame{"<main>", std::cref(main)});
	}

	auto Run() -> tl::expected<u32, RuntimeError> {
		return 0;
	}

private:
	static auto Print(std::string_view text) -> void {
		fmt::print("{}\n", text);
	}
	static auto Sqrt(float f) -> float {
		return static_cast<float>(sqrt(f));
	}
	static auto Sin(float f) -> float {
		return static_cast<float>(sin(f));
	}
	static auto Cos(float f) -> float {
		return static_cast<float>(cos(f));
	}
	static auto Tan(float f) -> float {
		return static_cast<float>(tan(f));
	}

	auto DefineFunction(const ASTNode& funcDefNode) -> void {
		auto& nameNode = *funcDefNode.children[0];
		auto& funcName = std::get<std::string>(*nameNode.extraData);
		functionDefs.insert({funcName, LuaFunction{funcDefNode}});
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

auto LuNI::RunProgram(
	argparse::ArgumentParser& args,
	const BytecodeProgram& opcodes
) -> void {
	// TODO
}


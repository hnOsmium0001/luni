#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>
#include <utility>
#include <optional>
#include <variant>
#include <functional>
#include <type_traits>

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;
using f32 = float;
using f64 = double;
using usize = size_t;

namespace LuNI {

template <class T, class Func>
auto operator|(const std::optional<T>& opt, Func&& func) -> std::optional<decltype(func(*opt))> {
	return opt ? func(*opt) : std::nullopt;
}
template <class Func, class... Ts>
auto Fmap(Func&& func, const std::optional<Ts>&... opts) -> std::optional<decltype(func((*opts)...))> {
	return (... && opts) ? func((*opts)...) : std::nullopt;
}

template <class T, class Func>
auto operator>>(const std::optional<T>& opt, Func&& func) -> decltype(func(*opt)) {
	return opt ? func(*opt) : std::nullopt;
}
template <class Func, class... Ts>
auto Bind(Func&& func, const std::optional<Ts>&... opts) -> decltype(func((*opts)...)) {
	return (... && opts) ? func((*opts)...) : std::nullopt;
}

template <class... Ts> struct Overloaded : Ts... { using Ts::operator()...; };
template <class... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;

template <class F>
class ScopeGuard {
private:
	F function;
	bool cancelled = false;

public:
	ScopeGuard() = delete;
	explicit ScopeGuard(F&& function) noexcept
		: function{ std::move(function) } {
	}
	ScopeGuard(const ScopeGuard&) = delete;
	ScopeGuard& operator=(const ScopeGuard&) = delete;
	ScopeGuard(ScopeGuard&& that)
		: function{ std::move(that.function) } {
		that.Cancel();
	}
	ScopeGuard& operator=(ScopeGuard&& that) {
		this->function = std::move(that.function);
		that.Cancel();
	}
	~ScopeGuard() noexcept {
		if (!cancelled) {
			function();
		}
	}

	auto Cancel() -> void { this->cancelled = true; }
};

// Parser/Lexer错误，包含错误ID和信息
struct StandardError {
	u32 id;
	std::string msg;
};

/// 解释器运行时错误
struct RuntimeError {
	// TODO stacktrace
};

namespace ErrorCodes {
	constexpr u32 INPUT_FILE_NOT_FOUND = 0;

	constexpr u32 PARSER_EXPECTED_IDENTIFIER = 100;
	constexpr u32 PARSER_EXPECTED_OPERATOR = 101;
	// TODO
} // namespace ErrorCodes

} // namespace LuNI


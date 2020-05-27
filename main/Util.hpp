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

template <typename T, typename Func>
auto operator|(const std::optional<T>& opt, Func&& func) -> std::optional<decltype(func(*opt))> {
	return opt ? func(*opt) : std::nullopt;
}
template <typename Func, typename... Ts>
auto Fmap(Func&& func, const std::optional<Ts>&... opts) -> std::optional<decltype(func((*opts)...))> {
	return (... && opts) ? func((*opts)...) : std::nullopt;
}

template <typename T, typename Func>
auto operator>>(const std::optional<T>& opt, Func&& func) -> decltype(func(*opt)) {
	return opt ? func(*opt) : std::nullopt;
}
template <typename Func, typename... Ts>
auto Bind(Func&& func, const std::optional<Ts>&... opts) -> decltype(func((*opts)...)) {
	return (... && opts) ? func((*opts)...) : std::nullopt;
}

template <typename F>
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
	ScopeGuard(ScopeGuard&&) = delete;
	ScopeGuard& operator=(ScopeGuard&&) = delete;
	~ScopeGuard() noexcept {
		if (!cancelled) {
			function();
		}
	}

	auto Cancel() -> void { this->cancelled = true; }
};

template <typename Iterator>
class Slice {
private:
	Iterator beginIt;
	usize size;

public:
	static auto None(Iterator end) -> Slice {
		return Slice{end, 0};
	}
	static auto Between(Iterator begin, Iterator end) -> Slice {
		return Slice{begin, std::distance(begin, end)};
	}
	static auto At(Iterator begin, usize size) -> Slice {
		return Slice{begin, size};
	}

	Slice(Iterator begin, usize size)
		: beginIt{ begin }, size{ size } {}

	Slice(const Slice& that) = default;
	Slice& operator=(const Slice& that) = default;
	Slice(Slice&& that) = default;
	Slice& operator=(Slice&& that) = default;

	auto operator[](usize index) -> typename std::iterator_traits<Iterator>::reference {
		return beginIt[index];
	}
	auto begin() const -> Iterator { return beginIt; }
	auto end() const -> Iterator { return beginIt + size; }

	auto Size() -> usize { return size; }
};

template <typename Container>
using RegularSlice = Slice<typename Container::iterator>;
template <typename Container>
using ConstSlice = Slice<typename Container::const_iterator>;

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


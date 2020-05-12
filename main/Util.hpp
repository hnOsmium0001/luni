#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
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


namespace LuNI {

template<class... Ts> struct Overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;

template <typename T, typename E>
class Result {
private:
	std::variant<T, E> data;

public:
	static_assert(!std::is_same<T, E>::value);

	Result(T t) noexcept : data{ std::move(t) } {}
	Result(E e) noexcept : data{ std::move(e) } {}
	
	Result(Result&& that) noexcept = default;
	Result& operator=(Result&& that) noexcept = default;
	~Result() noexcept = default;

	auto IsOk() const -> bool { return data.index() == 0; }
	auto IsErr() const -> bool { return data.index() == 1; }
	
	auto Ok() -> T& { return std::get<T>(data); }
	auto Ok() const -> const T& { return std::get<T>(data); }
	auto Err() -> E& { return std::get<E>(data); }
	auto Err() const -> const E& { return std::get<E>(data); }
	auto Data() -> std::variant<T, E>& { return data; }
	auto Data() const -> const std::variant<T, E>& { return data; }

	bool operator==(const Result<T, E>& that) const { return this->data == that.data; }
	operator bool() const { return this->IsOk(); }
};

/// General error wrapper around error ID and error message
struct StandardError {
	u32 id;
	std::string msg;
};

inline auto Err(u32 id, std::string msg) -> StandardError {
	return StandardError{std::move(id), std::move(msg)};
}

namespace ErrorCodes {
	constexpr u32 INPUT_FILE_NOT_FOUND = 0;
} // namespace ErrorCodes

} // namespace LuNI

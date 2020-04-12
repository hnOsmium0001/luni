#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

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



} // namespace LuNI

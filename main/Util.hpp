#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#define STRINGIFY_IMPL(text) #text
#define STRINGIFY(text) STRINGIFY_IMPL(text)

#define CONCAT_IMPL(a, b) a##b
#define CONCAT(a, b) CONCAT_IMPL(a, b)
#define CONCAT_3(a, b, c) CONCAT(a, CONCAT(b, c))
#define CONCAT_4(a, b, c, d) CONCAT(CONCAT(a, b), CONCAT(c, d))

#define UNIQUE_NAME(prefix) CONCAT(prefix, __COUNTER__)
#define UNIQUE_NAME_LINE(prefix) CONCAT(prefix, __LINE__)
#define DISCARD UNIQUE_NAME(_discard)

#define UNUSED(x) (void)x;

#if defined(_MSC_VER)
#	define UNREACHABLE __assume(0)
#elif defined(__GNUC__) || defined(__clang__)
#	define UNREACHABLE __builtin_unreachable()
#else
#	define UNREACHABLE
#endif

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

template <class... Ts>
struct Overloaded : Ts... { using Ts::operator()...; };

template <class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

} // namespace LuNI

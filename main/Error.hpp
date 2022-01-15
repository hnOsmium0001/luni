#pragma once

#include "Util.hpp"

namespace LuNI {

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

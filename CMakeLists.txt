cmake_minimum_required(VERSION 3.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(LuaInterpreter)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

add_executable(luni
	src/Util.hpp
	src/Util.cpp
	src/Lexer.hpp
	src/Lexer.cpp
	src/Parser.hpp
	src/Parser.cpp
	src/CodeGen.hpp
	src/CodeGen.cpp
	src/Main.cpp
)
target_link_libraries(luni)

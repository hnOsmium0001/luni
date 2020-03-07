#include <algorithm>
#include <array>
#include <cctype>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "aliases.hpp"

static inline auto ltrim(std::string &s) -> void {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(),
                                  [](int ch) { return !std::isspace(ch); }));
}

static inline auto rtrim(std::string &s) -> void {
  s.erase(std::find_if(s.rbegin(), s.rend(),
                       [](int ch) { return !std::isspace(ch); })
              .base(),
          s.end());
}

static inline auto trim(std::string &s) -> void {
  ltrim(s);
  rtrim(s);
}

constexpr usize LOC_FILE_NAME = 0;
constexpr usize LOC_DUMP_LOG = 1;
constexpr usize NUM_LOC = 2;

// TODO use tclap
auto ParseArguments(i32 argc, char **argv)
    -> std::pair<std::array<std::string, NUM_LOC>, std::vector<std::string>> {
  auto arguments = std::array<std::string, NUM_LOC>{};
  auto errors = std::vector<std::string>{};

  if (argc > 1)
    arguments[LOC_FILE_NAME] = argv[1];
  else
    errors.push_back("No input file specified");

  for (auto i = usize{2}; i < argc; ++i) {
    auto arg = std::string{argv[i]};
    if (arg == "-l" || arg == "--log") {
      arguments[LOC_DUMP_LOG] = "--log";
    }
  }

  return {arguments, errors};
}

i32 main(i32 argc, char **argv) {
  auto [args, errors] = ParseArguments(argc, argv);
  if (errors.size() > 0) {
    std::cout << errors.size() << " errors generated: \n";
    for (auto &msg : errors) std::cout << "\t" << msg << "\n";
    std::cout << "Aborting.\n";
    return -1;
  }

  std::cout << args[LOC_FILE_NAME] << "\n";
  std::cout << args[LOC_DUMP_LOG] << "\n";
  return 0;
}
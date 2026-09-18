cmake_minimum_required(VERSION 3.21)
project(puka VERSION 0.4.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra)
endif()

include(FetchContent)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# Vendored and statically linked so `puka` is a single self-contained binary
# -- no system package for FTXUI is assumed to exist.
set(FTXUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(FTXUI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(FTXUI_BUILD_DOCS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
  GIT_TAG        v7.0.3
  GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(ftxui)
# NOTE: FetchContent (not find_package) only gives us the raw `component` /
# `dom` / `screen` targets plus the umbrella `ftxui::ftxui` ALIAS -- the
# per-module `ftxui::component` / `ftxui::dom` / `ftxui::screen` aliases are
# only created by FTXUI's *install* export, which FetchContent never runs.
# Link against `ftxui::ftxui` everywhere.

# --- tree-sitter core + grammars (syntax highlighting) --------------------
enable_language(C)   # grammar parser.c/scanner.c sources are plain C
if(POLICY CMP0169)
  cmake_policy(SET CMP0169 OLD)  # legacy FetchContent_Populate is fine, just soft-deprecated
endif()

FetchContent_Declare(tree-sitter
  GIT_REPOSITORY https://github.com/tree-sitter/tree-sitter
  GIT_TAG        v0.27.0
  GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(tree-sitter)
# Safe to add_subdirectory normally: tree-sitter core's own CMakeLists.txt has
# no CLI-dependent custom_command (verified directly), unlike every grammar
# repo below.

# Fetches a grammar repo's SOURCE ONLY -- deliberately never configures its
# own CMakeLists.txt. Every grammar repo's build file contains
# `add_custom_command(OUTPUT src/parser.c DEPENDS src/grammar.json COMMAND
# tree-sitter generate ...)`; since both files are checked into git and get
# checkout-time mtimes, Make/Ninja can non-deterministically decide parser.c
# is stale and try to invoke the tree-sitter CLI, which won't be installed --
# breaking the build. We compile the checked-in parser.c/scanner.c ourselves
# instead (see puka_add_ts_grammar_lib below).
function(puka_fetch_ts_source name repo_url tag out_var)
  set(fc_name "fc_${name}")
  FetchContent_Declare(${fc_name}
    GIT_REPOSITORY ${repo_url}
    GIT_TAG        ${tag}
    GIT_SHALLOW    TRUE)
  FetchContent_GetProperties(${fc_name})
  if(NOT ${fc_name}_POPULATED)
    FetchContent_Populate(${fc_name})
  endif()
  set(${out_var} "${${fc_name}_SOURCE_DIR}" PARENT_SCOPE)
endfunction()

# Compiles a grammar's checked-in src/parser.c (+ src/scanner.c) directly,
# bypassing its own CMakeLists.txt entirely.
function(puka_add_ts_grammar_lib target_name grammar_root)
  add_library(${target_name} STATIC "${grammar_root}/src/parser.c")
  if(EXISTS "${grammar_root}/src/scanner.c")
    target_sources(${target_name} PRIVATE "${grammar_root}/src/scanner.c")
  endif()
  target_include_directories(${target_name} PRIVATE "${grammar_root}/src")
  target_compile_options(${target_name} PRIVATE -w)  # generated parsers are warning-noisy; not our code
  set_target_properties(${target_name} PROPERTIES
    C_STANDARD 11
    POSITION_INDEPENDENT_CODE ON)
endfunction()

# One-repo-one-grammar convenience wrapper.
function(puka_add_ts_grammar target_name repo_url tag)
  puka_fetch_ts_source(${target_name} ${repo_url} ${tag} _root)
  puka_add_ts_grammar_lib(${target_name} "${_root}")
endfunction()

puka_add_ts_grammar(tree-sitter-c          https://github.com/tree-sitter/tree-sitter-c          v0.24.2)
puka_add_ts_grammar(tree-sitter-cpp        https://github.com/tree-sitter/tree-sitter-cpp        v0.23.4)
puka_add_ts_grammar(tree-sitter-python     https://github.com/tree-sitter/tree-sitter-python     v0.25.0)
puka_add_ts_grammar(tree-sitter-javascript https://github.com/tree-sitter/tree-sitter-javascript v0.25.0)
puka_add_ts_grammar(tree-sitter-json       https://github.com/tree-sitter/tree-sitter-json       v0.24.8)
puka_add_ts_grammar(tree-sitter-bash       https://github.com/tree-sitter/tree-sitter-bash       v0.25.1)

# tree-sitter-typescript is one repo containing TWO grammars (typescript/ and
# tsx/ subdirectories); fetch it once, build both libraries from it.
puka_fetch_ts_source(tree-sitter-typescript
  https://github.com/tree-sitter/tree-sitter-typescript v0.23.2 _ts_root)
puka_add_ts_grammar_lib(tree-sitter-typescript "${_ts_root}/typescript")
puka_add_ts_grammar_lib(tree-sitter-tsx        "${_ts_root}/tsx")

# --- libgit2 (Source Control sidebar; read-only status/branch queries only) --
# Every cache variable below is verified against libgit2 v1.9.7's actual
# root CMakeLists.txt, not recalled from older docs -- several option NAMES
# have drifted: USE_REGEX is now REGEX_BACKEND, USE_COMPRESSION is now
# USE_BUNDLED_ZLIB.
set(BUILD_TESTS      OFF       CACHE BOOL   "" FORCE)  # libgit2's own Clar test suite
set(BUILD_CLI        OFF       CACHE BOOL   "" FORCE)  # the `git2` CLI companion binary -- unused
set(BUILD_EXAMPLES   OFF       CACHE BOOL   "" FORCE)
set(BUILD_FUZZERS    OFF       CACHE BOOL   "" FORCE)
set(USE_SSH          OFF       CACHE STRING "" FORCE)  # no clone/fetch/push -- no transport needed
set(USE_HTTPS        OFF       CACHE STRING "" FORCE)  # skips OpenSSL/mbedTLS entirely
set(REGEX_BACKEND    "builtin" CACHE STRING "" FORCE)  # vendors bundled PCRE2 -- no system regex lib
set(USE_BUNDLED_ZLIB ON        CACHE STRING "" FORCE)  # vendors zlib -- no system zlib

FetchContent_Declare(libgit2
  GIT_REPOSITORY https://github.com/libgit2/libgit2
  GIT_TAG        v1.9.7
  GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(libgit2)
# GOTCHA (verified by reading src/libgit2/CMakeLists.txt at this tag): the
# real consumable library target is `libgit2package` -- NOT `git2` or
# `libgit2` (an internal OBJECT library also named `libgit2` exists but is
# not independently link-usable). Its only INTERFACE include directory is
# $<INSTALL_INTERFACE:...>, which is inert under FetchContent (no install
# step runs), so linking it alone gives consumers ZERO include dirs -- see
# src/git/CMakeLists.txt, which adds ${libgit2_SOURCE_DIR}/include manually.
# That's sufficient on its own (no need for the CMake-generated
# ${libgit2_BINARY_DIR}/include copy) because libgit2 ships a static
# fallback include/git2/experimental.h specifically for non-CMake/bespoke
# build consumers like this one.

add_subdirectory(src)

option(PUKA_BUILD_TESTS "Build unit tests" ON)
if(PUKA_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

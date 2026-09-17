#pragma once
#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace puka {

struct SearchHit {
  std::filesystem::path file;
  size_t line;    // 1-based, exactly as rg/grep/the scan fallback report it
  size_t column;  // 1-based byte offset; 1 when the tool doesn't report one (grep)
  std::string preview;
};

enum class SearchTool { Ripgrep, Grep, PlainScan };

struct SearchResult {
  std::vector<SearchHit> hits;
  bool truncated = false;
  SearchTool tool_used = SearchTool::Ripgrep;
  // Empty on a normal rg search. Set when a tier fell back, or when a tool
  // ran but exited with a genuine error (not just "0 matches").
  std::string note;
};

inline constexpr size_t kDefaultMaxResults = 500;

// Synchronous, workspace-wide substring search. Tries `rg --vimgrep`, then
// `grep -rn`, then a pure recursive-directory-scan, stopping at the first
// tool that's actually installed. `root` is canonicalized internally, so
// every SearchHit::file is an absolute path regardless of the caller's CWD.
SearchResult RunSearch(const std::string& query, const std::filesystem::path& root,
                       size_t max_results = kDefaultMaxResults);

// Pure parsing logic, unit-testable without a live subprocess. Both reject
// (return nullopt) any line that doesn't have the required ':'-delimited
// fields with numeric line/column values -- this is what defensively skips
// rg's own "<path>: binary file matches (...)" summary lines and any other
// stray tool output.
std::optional<SearchHit> ParseRipgrepVimgrepLine(std::string_view line);
std::optional<SearchHit> ParseGrepLine(std::string_view line);

// Pure cap/truncation logic, unit-testable in isolation.
SearchResult CapResults(std::vector<SearchHit> hits, size_t max_results);

}  // namespace puka

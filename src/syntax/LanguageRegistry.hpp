#pragma once
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "syntax/TreeSitterFwd.hpp"

namespace puka {

struct LanguageSpec {
  std::string_view name;                      // "c", "cpp", "python", ...
  std::vector<std::string_view> extensions;   // lowercase, including the dot
  const TSLanguage* (*language_fn)();
  std::string highlights_query;               // owned; concatenated at registry construction
};

// Meyers singleton: this is process-wide, stateless-per-document registry
// data plus a shared, expensive-to-compile TSQuery* cache, and it must be
// reachable both from Document::Open() (a static factory with no `this`)
// and from anywhere else without threading a reference through
// DocumentManager::OpenFile(path) (which only takes a path). There is
// legitimately exactly one of these for the whole process, and its state
// (compiled queries) is safe to share read-only across every open Document.
class LanguageRegistry {
 public:
  static LanguageRegistry& Instance();

  // Returns nullptr for an unrecognized extension -- a clean no-op, not an
  // error, since puka opens arbitrary files regardless of language support.
  const LanguageSpec* Detect(const std::filesystem::path& path) const;

  // Compiles (once) and caches the TSQuery* for `spec`, reused by every
  // Document of that language. Returns nullptr if compilation failed (never
  // crashes -- callers treat nullptr as "no highlighting for this language").
  TSQuery* CompiledQuery(const LanguageSpec& spec);

  LanguageRegistry(const LanguageRegistry&) = delete;
  LanguageRegistry& operator=(const LanguageRegistry&) = delete;

 private:
  LanguageRegistry();

  std::vector<LanguageSpec> specs_;  // stable addresses: built once in the ctor, never mutated after
  std::unordered_map<const LanguageSpec*, TSQuery*> compiled_queries_;
};

}  // namespace puka

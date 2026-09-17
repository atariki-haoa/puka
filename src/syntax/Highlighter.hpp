#pragma once
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "editor/BufferEdit.hpp"
#include "syntax/TreeSitterFwd.hpp"

namespace puka {

struct LanguageSpec;  // full definition in LanguageRegistry.hpp

struct HighlightSpan {
  size_t start_byte;
  size_t end_byte;
  // Points into the compiled TSQuery's own capture-name table, owned by
  // LanguageRegistry for the process lifetime -- never into transient
  // buffer/source text, so this stays valid indefinitely.
  std::string_view capture_name;
};

// Owns one Document's TSParser/TSTree. Deliberately non-copyable and
// non-movable (raw tree-sitter pointers) -- Document only ever holds it via
// unique_ptr<Highlighter>, so Highlighter's own move-constructibility is
// irrelevant to Document's movability.
class Highlighter {
 public:
  explicit Highlighter(const LanguageSpec& spec);
  ~Highlighter();
  Highlighter(const Highlighter&) = delete;
  Highlighter& operator=(const Highlighter&) = delete;
  Highlighter(Highlighter&&) = delete;
  Highlighter& operator=(Highlighter&&) = delete;

  // Parses `full_text` from scratch (no previous tree). Call once after Open().
  void ReparseFull(std::string_view full_text);

  // Applies one edit delta and reparses `new_full_text` (the buffer's
  // current, already-mutated content), using the previous TSTree as a
  // diffing hint via ts_tree_edit + ts_parser_parse_string.
  void Edit(const BufferEdit& edit, std::string_view new_full_text);

  // Flat, non-overlapping, position-ordered spans covering [start_byte,
  // end_byte). Empty if this language has no compiled query.
  std::vector<HighlightSpan> SpansForByteRange(size_t start_byte, size_t end_byte) const;

 private:
  const LanguageSpec* spec_;
  TSParser* parser_;
  TSTree* tree_ = nullptr;
  std::string source_text_;  // cached full text, used for #match?/#not-match? regex evaluation
};

}  // namespace puka

#pragma once
#include <string_view>

// Vendored verbatim from tree-sitter/tree-sitter-json v0.24.8 queries/highlights.scm
// Do not hand-edit; regenerate from the pinned tag if it ever needs updating.

namespace puka {

inline constexpr std::string_view kJsonHighlightsScm = R"TSQUERY(
(pair
  key: (_) @string.special.key)

(string) @string

(number) @number

[
  (null)
  (true)
  (false)
] @constant.builtin

(escape_sequence) @escape

(comment) @comment
)TSQUERY";

}  // namespace puka

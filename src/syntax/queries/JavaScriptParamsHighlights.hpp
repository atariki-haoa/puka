#pragma once
#include <string_view>

// Vendored verbatim from tree-sitter/tree-sitter-javascript v0.25.0 queries/highlights-params.scm
// Do not hand-edit; regenerate from the pinned tag if it ever needs updating.

namespace puka {

inline constexpr std::string_view kJavaScriptParamsHighlightsScm = R"TSQUERY(
(formal_parameters
  [
    (identifier) @variable.parameter
    (array_pattern
      (identifier) @variable.parameter)
    (object_pattern
      [
        (pair_pattern value: (identifier) @variable.parameter)
        (shorthand_property_identifier_pattern) @variable.parameter
      ])
  ]
)
)TSQUERY";

}  // namespace puka

#pragma once
#include <string_view>

// Vendored verbatim from tree-sitter/tree-sitter-typescript v0.23.2 queries/highlights.scm
// Do not hand-edit; regenerate from the pinned tag if it ever needs updating.

namespace puka {

inline constexpr std::string_view kTypeScriptHighlightsScm = R"TSQUERY(
; Types

(type_identifier) @type
(predefined_type) @type.builtin

((identifier) @type
 (#match? @type "^[A-Z]"))

(type_arguments
  "<" @punctuation.bracket
  ">" @punctuation.bracket)

; Variables

(required_parameter (identifier) @variable.parameter)
(optional_parameter (identifier) @variable.parameter)

; Keywords

[ "abstract"
  "declare"
  "enum"
  "export"
  "implements"
  "interface"
  "keyof"
  "namespace"
  "private"
  "protected"
  "public"
  "type"
  "readonly"
  "override"
  "satisfies"
] @keyword
)TSQUERY";

}  // namespace puka

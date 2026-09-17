#pragma once
#include <string_view>

// Vendored verbatim from tree-sitter/tree-sitter-bash v0.25.1 queries/highlights.scm
// Do not hand-edit; regenerate from the pinned tag if it ever needs updating.

namespace puka {

inline constexpr std::string_view kBashHighlightsScm = R"TSQUERY(
[
  (string)
  (raw_string)
  (heredoc_body)
  (heredoc_start)
] @string

(command_name) @function

(variable_name) @property

[
  "case"
  "do"
  "done"
  "elif"
  "else"
  "esac"
  "export"
  "fi"
  "for"
  "function"
  "if"
  "in"
  "select"
  "then"
  "unset"
  "until"
  "while"
] @keyword

(comment) @comment

(function_definition name: (word) @function)

(file_descriptor) @number

[
  (command_substitution)
  (process_substitution)
  (expansion)
]@embedded

[
  "$"
  "&&"
  ">"
  ">>"
  "<"
  "|"
] @operator

(
  (command (_) @constant)
  (#match? @constant "^-")
)
)TSQUERY";

}  // namespace puka

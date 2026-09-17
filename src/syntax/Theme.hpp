#pragma once
#include <string_view>

#include <ftxui/dom/elements.hpp>

namespace puka {

// Applies the capture-name's color/decorator (approximating VSCode Dark+) to
// `element`. Falls back through dotted-name prefixes (e.g.
// "function.method" -> "function") before giving up and leaving `element`
// unstyled at the terminal's default foreground -- also how @variable,
// @operator, @punctuation.* etc. intentionally stay uncolored, matching
// Dark+ (only capture names actually produced by the vendored queries have
// an entry).
ftxui::Element ApplyCaptureStyle(std::string_view capture_name, ftxui::Element element);

}  // namespace puka

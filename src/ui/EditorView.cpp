#include "ui/EditorView.hpp"

#include <algorithm>

#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/terminal.hpp>

#include "syntax/Theme.hpp"

namespace puka {
using namespace ftxui;

namespace {

struct Fragment {
  size_t start_col, end_col;
  std::string_view capture;
  bool is_cursor = false;
};

// Builds one line's colored fragments (from the document's highlight spans,
// if any) and, for the cursor's own line, splices in a one-column fragment
// at the cursor position so it can be rendered inverted -- without losing
// that column's original capture color on either side of the split.
Element RenderLine(const Document& doc, size_t row, bool is_current) {
  std::string_view line_text = doc.buffer().Line(row);
  size_t line_start_byte = doc.buffer().ByteOffset(row, 0);

  std::vector<Fragment> fragments;
  size_t pos = 0;
  for (const auto& span : doc.HighlightSpans(line_start_byte, line_start_byte + line_text.size())) {
    size_t s = std::min(span.start_byte - line_start_byte, line_text.size());
    size_t e = std::min(span.end_byte - line_start_byte, line_text.size());
    if (s > pos) fragments.push_back({pos, s, ""});
    if (e > s) fragments.push_back({s, e, span.capture_name});
    pos = std::max(pos, e);
  }
  if (pos < line_text.size()) fragments.push_back({pos, line_text.size(), ""});

  if (is_current) {
    size_t cursor_col = std::min(doc.cursor().col, line_text.size());
    std::vector<Fragment> spliced;
    bool inserted = false;
    for (const auto& f : fragments) {
      if (!inserted && cursor_col >= f.start_col && cursor_col < f.end_col) {
        if (cursor_col > f.start_col) spliced.push_back({f.start_col, cursor_col, f.capture, false});
        spliced.push_back({cursor_col, cursor_col + 1, f.capture, true});
        if (cursor_col + 1 < f.end_col) spliced.push_back({cursor_col + 1, f.end_col, f.capture, false});
        inserted = true;
      } else {
        spliced.push_back(f);
      }
    }
    if (!inserted) {
      // Cursor past end of line (or the line is empty) -- a phantom cell.
      spliced.push_back({cursor_col, cursor_col + 1, "", true});
    }
    fragments = std::move(spliced);
  }

  Elements parts;
  for (const auto& f : fragments) {
    std::string segment = (f.is_cursor && f.start_col >= line_text.size())
                               ? " "
                               : std::string(line_text.substr(f.start_col, f.end_col - f.start_col));
    Element el = text(segment);
    if (!f.capture.empty()) el = ApplyCaptureStyle(f.capture, el);
    if (f.is_cursor) el = el | inverted;
    parts.push_back(el);
  }
  return parts.empty() ? text("") : hbox(std::move(parts));
}

}  // namespace

EditorView::EditorView(DocumentManager& documents) : documents_(documents) {}

Element EditorView::OnRender() {
  if (documents_.empty()) {
    return vbox({filler(), hcenter(text("No file open") | dim), filler()});
  }

  const auto& docs = documents_.documents();
  Elements tabs;
  for (size_t i = 0; i < docs.size(); ++i) {
    std::string label = " " + docs[i].DisplayName() + (docs[i].dirty() ? " *" : "") + " ";
    Element tab = text(label);
    if (i == documents_.active_index()) tab = tab | inverted | bold;
    tabs.push_back(tab);
  }
  Element tab_bar = hbox(std::move(tabs));
  bool focused = Focused();
  if (focused) tab_bar = tab_bar | color(Color::Cyan);

  const Document& doc = docs[documents_.active_index()];
  size_t total_lines = doc.buffer().LineCount();
  size_t digits = std::to_string(total_lines).size();
  int gutter_width = static_cast<int>(digits) + 1;

  Elements lines;
  for (size_t row = 0; row < total_lines; ++row) {
    bool is_current = (row == doc.cursor().row);

    std::string number = std::to_string(row + 1);
    std::string gutter_text(digits - number.size(), ' ');
    gutter_text += number;
    gutter_text += ' ';
    Element gutter = text(gutter_text);
    if (!is_current) gutter = gutter | dim;

    // The cursor is rendered as an inverted (block-style) character spliced
    // into the colored fragments, rather than relying on the terminal's own
    // cursor, which FTXUI doesn't otherwise place for us here.
    Element code = RenderLine(doc, row, is_current);
    lines.push_back(hbox({gutter, code}));
  }

  Element content = vbox(std::move(lines)) |
                    focusPosition(gutter_width + static_cast<int>(doc.cursor().col),
                                  static_cast<int>(doc.cursor().row)) |
                    frame | flex;

  std::string status =
      "Ln " + std::to_string(doc.cursor().row + 1) + ", Col " + std::to_string(doc.cursor().col + 1);
  Element status_bar = hbox({filler(), text(status) | dim, text(" ")});

  // The colored divider right under the tab bar is the main "you just
  // switched panes" signal -- it flips the instant focus changes, no
  // animation timing/state needed for something this immediate.
  Element top_separator = separator() | color(focused ? Color::Cyan : Color::GrayDark);

  return vbox({tab_bar, top_separator, content, separator(), status_bar});
}

bool EditorView::OnEvent(Event event) {
  if (documents_.empty()) return false;
  Document* doc = documents_.Active();

  if (event == Event::ArrowLeft) { doc->MoveLeft(); return true; }
  if (event == Event::ArrowRight) { doc->MoveRight(); return true; }
  if (event == Event::ArrowUp) { doc->MoveUp(); return true; }
  if (event == Event::ArrowDown) { doc->MoveDown(); return true; }
  if (event == Event::Home) { doc->MoveHome(); return true; }
  if (event == Event::End) { doc->MoveEnd(); return true; }
  if (event == Event::PageUp || event == Event::PageDown) {
    size_t page = static_cast<size_t>(std::max(1, Terminal::Size().dimy - 4));
    if (event == Event::PageUp) doc->MovePageUp(page);
    else doc->MovePageDown(page);
    return true;
  }
  if (event == Event::Backspace) { doc->DeleteBackward(); return true; }
  if (event == Event::Delete) { doc->DeleteForward(); return true; }
  if (event == Event::Return) { doc->InsertNewline(); return true; }
  if (event.is_character()) {
    doc->InsertText(event.character());
    return true;
  }
  return false;
}

}  // namespace puka

#include "ui/EditorView.hpp"

#include <algorithm>
#include <cctype>
#include <optional>
#include <utility>

#include <ftxui/component/event.hpp>
#include <ftxui/component/mouse.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/terminal.hpp>

#include "syntax/Theme.hpp"

namespace puka {
using namespace ftxui;

namespace {

struct Fragment {
  size_t start_col, end_col;
  std::string_view capture;
  // Rendered inverted for the plain text cursor, or as a yellow highlight
  // when this fragment is a Ctrl+F match instead -- the two never coexist
  // on the same line (RenderLine only ever splices one or the other).
  bool is_cursor = false;
};

// Builds one line's colored fragments (from the document's highlight spans,
// if any). For the cursor's own line, splices in a one-column fragment at
// the cursor position so it can be rendered inverted -- without losing that
// column's original capture color on either side of the split. When
// `match_range` is set (only ever true for the current active Ctrl+F match's
// own line), that whole [start,end) span is splice-highlighted instead of
// the single-column cursor cell, so a multi-character match is visible as
// more than just where the cursor happens to land.
Element RenderLine(const Document& doc, size_t row, bool is_current,
                    std::optional<std::pair<size_t, size_t>> match_range) {
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

  if (match_range) {
    size_t start = std::min(match_range->first, line_text.size());
    size_t end = std::min(std::max(match_range->second, start + 1), line_text.size() + 1);
    std::vector<Fragment> spliced;
    bool inserted = false;
    for (const auto& f : fragments) {
      if (!inserted && start >= f.start_col && start < f.end_col) {
        if (start > f.start_col) spliced.push_back({f.start_col, start, f.capture, false});
        size_t seg_end = std::min(end, f.end_col);
        spliced.push_back({start, seg_end, f.capture, true});
        if (seg_end < f.end_col) spliced.push_back({seg_end, f.end_col, f.capture, false});
        inserted = true;
      } else {
        spliced.push_back(f);
      }
    }
    if (!inserted) spliced.push_back({start, start + 1, "", true});
    fragments = std::move(spliced);
  } else if (is_current) {
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
    if (f.is_cursor) el = el | (match_range ? bgcolor(Color::Yellow) | color(Color::Black) : inverted);
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
  tab_boxes_.assign(docs.size(), Box());
  Elements tabs;
  for (size_t i = 0; i < docs.size(); ++i) {
    std::string label = " " + docs[i].DisplayName() + (docs[i].dirty() ? " *" : "") + " ";
    Element tab = text(label);
    if (i == documents_.active_index()) tab = tab | inverted | bold;
    tab = tab | reflect(tab_boxes_[i]);
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
    std::optional<std::pair<size_t, size_t>> match_range;
    if (find_active_ && find_has_match_ && row == find_match_row_) {
      match_range = {find_match_start_col_, find_match_end_col_};
    }

    std::string number = std::to_string(row + 1);
    std::string gutter_text(digits - number.size(), ' ');
    gutter_text += number;
    gutter_text += ' ';
    Element gutter = text(gutter_text);
    if (!is_current) gutter = gutter | dim;

    // The cursor is rendered as an inverted (block-style) character spliced
    // into the colored fragments, rather than relying on the terminal's own
    // cursor, which FTXUI doesn't otherwise place for us here.
    Element code = RenderLine(doc, row, is_current, match_range);
    lines.push_back(hbox({gutter, code}));
  }

  Element content = vbox(std::move(lines)) |
                    focusPosition(gutter_width + static_cast<int>(doc.cursor().col),
                                  static_cast<int>(doc.cursor().row)) |
                    frame | flex;

  std::string status =
      "Ln " + std::to_string(doc.cursor().row + 1) + ", Col " + std::to_string(doc.cursor().col + 1);
  Element bottom_bar = find_active_ ? RenderFindBar() : hbox({filler(), text(status) | dim, text(" ")});

  // The colored divider right under the tab bar is the main "you just
  // switched panes" signal -- it flips the instant focus changes, no
  // animation timing/state needed for something this immediate.
  Element top_separator = separator() | color(focused ? Color::Cyan : Color::GrayDark);

  return vbox({tab_bar, top_separator, content, separator(), bottom_bar});
}

Element EditorView::RenderFindBar() {
  Element status;
  if (find_query_.empty()) {
    status = text("");
  } else if (find_has_match_) {
    status = text("Ln " + std::to_string(find_match_row_ + 1)) | dim;
  } else {
    status = text("no match") | color(Color::Red);
  }
  return hbox({text(" Find: ") | bold, text(find_query_) | underlined, text(" "), status, filler(),
               text("Enter/↓ next  ↑ prev  Esc close") | dim, text(" ")});
}

bool EditorView::OnEvent(Event event) {
  if (documents_.empty()) return false;

  if (event.is_mouse() && event.mouse().button == Mouse::Left &&
      event.mouse().motion == Mouse::Pressed) {
    for (size_t i = 0; i < tab_boxes_.size(); ++i) {
      if (!tab_boxes_[i].Contain(event.mouse().x, event.mouse().y)) continue;
      documents_.SetActiveIndex(i);
      TakeFocus();
      return true;
    }
  }

  Document* doc = documents_.Active();

  if (find_active_) {
    if (event == Event::Escape) {
      find_active_ = false;
      return true;
    }
    if (event == Event::Return || event == Event::ArrowDown) {
      FindNext();
      return true;
    }
    if (event == Event::ArrowUp) {
      FindPrev();
      return true;
    }
    if (event == Event::Backspace) {
      if (!find_query_.empty()) find_query_.pop_back();
      FindFromScratch();
      return true;
    }
    if (event.is_character()) {
      find_query_ += event.character();
      FindFromScratch();
      return true;
    }
    return true;  // swallow everything else while the find bar has focus
  }

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

void EditorView::ActivateFind() {
  if (documents_.empty()) return;
  Document* doc = documents_.Active();
  find_active_ = true;
  find_query_.clear();
  find_has_match_ = false;
  find_origin_byte_ = doc->buffer().ByteOffset(doc->cursor().row, doc->cursor().col);
  TakeFocus();
}

namespace {
std::string ToLowerCopy(std::string_view s) {
  std::string out(s);
  std::transform(out.begin(), out.end(), out.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return out;
}
}  // namespace

// Plain case-insensitive substring search over the whole buffer content
// (byte-oriented, same as Buffer itself -- see its own doc comment), mapping
// the hit back to row/col via Buffer::RowCol. Good enough for the file sizes
// a terminal editor actually opens; not trying to be a streaming/regex
// search engine.
void EditorView::SearchAndJump(Document& doc, size_t from_byte, bool forward) {
  find_has_match_ = false;
  if (find_query_.empty()) return;

  std::string content = ToLowerCopy(doc.buffer().ToString());
  std::string query = ToLowerCopy(find_query_);
  if (content.empty()) return;

  size_t pos;
  if (forward) {
    pos = content.find(query, from_byte);
    if (pos == std::string::npos) pos = content.find(query, 0);
  } else {
    pos = (from_byte == 0) ? std::string::npos : content.rfind(query, from_byte - 1);
    if (pos == std::string::npos) pos = content.rfind(query, content.size());
  }
  if (pos == std::string::npos) return;

  find_has_match_ = true;
  auto [row, col] = doc.buffer().RowCol(pos);
  find_match_row_ = row;
  find_match_start_col_ = col;
  find_match_end_col_ = col + find_query_.size();
  doc.cursor().row = row;
  doc.cursor().col = col;
}

void EditorView::FindFromScratch() {
  if (documents_.empty()) return;
  SearchAndJump(*documents_.Active(), find_origin_byte_, /*forward=*/true);
}

void EditorView::FindNext() {
  if (documents_.empty()) return;
  Document& doc = *documents_.Active();
  size_t from = find_has_match_ ? doc.buffer().ByteOffset(find_match_row_, find_match_end_col_)
                                 : find_origin_byte_;
  SearchAndJump(doc, from, /*forward=*/true);
}

void EditorView::FindPrev() {
  if (documents_.empty()) return;
  Document& doc = *documents_.Active();
  size_t from = find_has_match_ ? doc.buffer().ByteOffset(find_match_row_, find_match_start_col_)
                                 : find_origin_byte_;
  SearchAndJump(doc, from, /*forward=*/false);
}

}  // namespace puka

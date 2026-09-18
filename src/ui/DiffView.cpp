#include "ui/DiffView.hpp"

#include <algorithm>

#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/terminal.hpp>

namespace puka {
using namespace ftxui;

namespace {

constexpr int kGutterWidth = 5;

// `col_width` is a *fixed* width for both sides, computed once per render
// from the terminal size -- not `flex`. flex sizes a cell from its own
// content's Requirement, so two cells of different line lengths on the
// same row would each claim a different share of the row and the
// left/right divider would drift left/right from row to row (a long line
// on one side, short on the other) instead of staying put. A fixed width
// keeps the divider in the same column for every row and clips (rather
// than reflows) whatever doesn't fit, which is what actually fixes the
// "columns don't line up" / "the pane balloons to fill the screen on a
// big diff" reports -- both were downstream of using flex + unbounded
// Requirement instead of a hard, content-independent size here.
Element Side(bool present, int lineno, const std::string& text_content, bool changed, bool is_old,
             int col_width) {
  Element gutter = text(present ? std::to_string(lineno) : "") | size(WIDTH, EQUAL, kGutterWidth) |
                   dim;
  Element body = text(present ? text_content : "") | size(WIDTH, EQUAL, col_width);
  if (present && changed) body = body | color(is_old ? Color::Red : Color::Green);
  return hbox({gutter, text(" "), body});
}

}  // namespace

DiffView::DiffView(bool* show) : show_(show) {}

void DiffView::SetDiff(std::string title, GitFileDiff diff) {
  title_ = std::move(title);
  diff_ = std::move(diff);
  selected_ = 0;
  RebuildRows();
}

void DiffView::RebuildRows() {
  rows_.clear();
  const auto& lines = diff_.lines;
  size_t i = 0;
  while (i < lines.size()) {
    if (lines[i].origin == GitDiffLineOrigin::Context) {
      Row row;
      row.has_old = true;
      row.old_lineno = lines[i].old_lineno;
      row.old_text = lines[i].content;
      row.has_new = true;
      row.new_lineno = lines[i].new_lineno;
      row.new_text = lines[i].content;
      rows_.push_back(std::move(row));
      ++i;
      continue;
    }

    // A hunk lists a contiguous run of deletions immediately followed by a
    // contiguous run of additions for each changed region -- zip the two
    // runs into paired rows so a same-size before/after edit lines up side
    // by side, with leftover lines on the longer side left unpaired.
    size_t del_start = i;
    while (i < lines.size() && lines[i].origin == GitDiffLineOrigin::Deletion) ++i;
    size_t del_end = i;
    size_t add_start = i;
    while (i < lines.size() && lines[i].origin == GitDiffLineOrigin::Addition) ++i;
    size_t add_end = i;

    size_t del_count = del_end - del_start;
    size_t add_count = add_end - add_start;
    size_t pair_count = std::max(del_count, add_count);
    for (size_t k = 0; k < pair_count; ++k) {
      Row row;
      row.changed = true;
      if (k < del_count) {
        row.has_old = true;
        row.old_lineno = lines[del_start + k].old_lineno;
        row.old_text = lines[del_start + k].content;
      }
      if (k < add_count) {
        row.has_new = true;
        row.new_lineno = lines[add_start + k].new_lineno;
        row.new_text = lines[add_start + k].content;
      }
      rows_.push_back(std::move(row));
    }
  }
}

Element DiffView::OnRender() {
  // Fixed, terminal-relative box (not `flex`, see the Side() comment above
  // for why) -- always the same near-full-screen size regardless of diff
  // content, like a dedicated pane rather than a content-sized popup.
  int content_w = std::max(20, Terminal::Size().dimx - 2);   // minus | border
  int content_h = std::max(8, Terminal::Size().dimy - 2);    // minus | border
  int body_h = std::max(3, content_h - 2);                   // minus header + separator
  int col_w = std::max(8, (content_w - 2 * (kGutterWidth + 1) - 1) / 2);  // minus gutters/separator

  Element header = hbox({text(title_) | bold, filler(), text("Esc to close") | dim}) |
                   size(WIDTH, EQUAL, content_w);

  Element body;
  if (!diff_.available) {
    body = vbox({filler(), hcenter(text("Could not read this file's diff") | dim), filler()});
  } else if (diff_.binary) {
    body = vbox({filler(), hcenter(text("Binary file -- cannot show a text diff") | dim), filler()});
  } else if (rows_.empty()) {
    body = vbox({filler(), hcenter(text("No changes") | dim), filler()});
  } else {
    Elements body_rows;
    for (int i = 0; i < static_cast<int>(rows_.size()); ++i) {
      const auto& row = rows_[i];
      Element left = Side(row.has_old, row.old_lineno, row.old_text, row.changed, true, col_w);
      Element right = Side(row.has_new, row.new_lineno, row.new_text, row.changed, false, col_w);
      Element line = hbox({left, separator(), right});
      if (i == selected_) line = line | inverted;
      body_rows.push_back(line);
    }
    body = vbox(std::move(body_rows)) | focusPosition(0, selected_) | frame;
  }
  body = body | size(WIDTH, EQUAL, content_w) | size(HEIGHT, EQUAL, body_h);

  return vbox({header, separator(), body}) | border;
}

bool DiffView::OnEvent(Event event) {
  if (event == Event::Escape) {
    *show_ = false;
    return true;
  }
  if (event == Event::ArrowUp) {
    selected_ = std::max(0, selected_ - 1);
  } else if (event == Event::ArrowDown) {
    selected_ = std::min(static_cast<int>(rows_.size()) - 1, selected_ + 1);
  }
  return true;
}

}  // namespace puka

#include "editor/Document.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>

#include "syntax/LanguageRegistry.hpp"

namespace puka {

std::optional<Document> Document::Open(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) return std::nullopt;
  std::ostringstream ss;
  ss << in.rdbuf();

  Document doc(path, ss.str());
  if (const LanguageSpec* spec = LanguageRegistry::Instance().Detect(path)) {
    doc.highlighter_ = std::make_unique<Highlighter>(*spec);
    doc.highlighter_->ReparseFull(doc.buffer_.ToString());
  }
  return doc;
}

Document::Document(std::filesystem::path path, std::string content)
    : path_(std::move(path)), buffer_(std::move(content)) {}

std::string Document::DisplayName() const {
  return path_.empty() ? "untitled" : path_.filename().string();
}

bool Document::Save() {
  if (path_.empty()) return false;

  std::filesystem::path tmp_path = path_;
  tmp_path += ".puka.tmp";
  {
    std::ofstream out(tmp_path, std::ios::binary | std::ios::trunc);
    if (!out) return false;
    out << buffer_.ToString();
    if (!out) return false;
  }

  std::error_code ec;
  std::filesystem::rename(tmp_path, path_, ec);
  if (ec) {
    std::filesystem::remove(tmp_path, ec);
    return false;
  }
  buffer_.MarkSaved();
  return true;
}

std::vector<HighlightSpan> Document::HighlightSpans(size_t start_byte, size_t end_byte) const {
  if (!highlighter_) return {};
  return highlighter_->SpansForByteRange(start_byte, end_byte);
}

void Document::InsertText(std::string_view text) {
  BufferEdit edit = buffer_.InsertText(cursor_.row, cursor_.col, text);
  cursor_.row = edit.new_end_row;
  cursor_.col = edit.new_end_col;
  if (highlighter_) highlighter_->Edit(edit, buffer_.ToString());
}

void Document::InsertChar(char c) { InsertText(std::string_view(&c, 1)); }

void Document::InsertNewline() { InsertText("\n"); }

void Document::DeleteBackward() {
  if (cursor_.col > 0) {
    auto [text, edit] = buffer_.DeleteRange(cursor_.row, cursor_.col - 1, cursor_.row, cursor_.col);
    (void)text;
    cursor_.col -= 1;
    if (highlighter_) highlighter_->Edit(edit, buffer_.ToString());
  } else if (cursor_.row > 0) {
    size_t prev_len = buffer_.Line(cursor_.row - 1).size();
    auto [text, edit] = buffer_.DeleteRange(cursor_.row - 1, prev_len, cursor_.row, 0);
    (void)text;
    cursor_.row -= 1;
    cursor_.col = prev_len;
    if (highlighter_) highlighter_->Edit(edit, buffer_.ToString());
  }
}

void Document::DeleteForward() {
  size_t line_len = buffer_.Line(cursor_.row).size();
  if (cursor_.col < line_len) {
    auto [text, edit] = buffer_.DeleteRange(cursor_.row, cursor_.col, cursor_.row, cursor_.col + 1);
    (void)text;
    if (highlighter_) highlighter_->Edit(edit, buffer_.ToString());
  } else if (cursor_.row + 1 < buffer_.LineCount()) {
    auto [text, edit] = buffer_.DeleteRange(cursor_.row, cursor_.col, cursor_.row + 1, 0);
    (void)text;
    if (highlighter_) highlighter_->Edit(edit, buffer_.ToString());
  }
}

void Document::DeleteLine() {
  size_t row = cursor_.row;
  size_t line_count = buffer_.LineCount();

  size_t row0, col0, row1, col1;
  if (line_count == 1) {
    // The only line in the buffer -- nothing to join, just clear it.
    row0 = 0;
    col0 = 0;
    row1 = 0;
    col1 = buffer_.Line(0).size();
  } else if (row + 1 < line_count) {
    // Consume this line plus the newline after it, so the line below slides
    // up into its place.
    row0 = row;
    col0 = 0;
    row1 = row + 1;
    col1 = 0;
  } else {
    // Last line -- there's no newline after it to consume, so consume the
    // one before it instead.
    row0 = row - 1;
    col0 = buffer_.Line(row - 1).size();
    row1 = row;
    col1 = buffer_.Line(row).size();
  }

  auto [text, edit] = buffer_.DeleteRange(row0, col0, row1, col1);
  (void)text;
  cursor_.row = row0;
  cursor_.col = col0;
  if (highlighter_) highlighter_->Edit(edit, buffer_.ToString());
}

void Document::MoveLeft() {
  if (cursor_.col > 0) {
    cursor_.col -= 1;
  } else if (cursor_.row > 0) {
    cursor_.row -= 1;
    cursor_.col = buffer_.Line(cursor_.row).size();
  }
}

void Document::MoveRight() {
  if (cursor_.col < buffer_.Line(cursor_.row).size()) {
    cursor_.col += 1;
  } else if (cursor_.row + 1 < buffer_.LineCount()) {
    cursor_.row += 1;
    cursor_.col = 0;
  }
}

void Document::MoveUp() {
  if (cursor_.row == 0) return;
  cursor_.row -= 1;
  cursor_.col = std::min(cursor_.col, buffer_.Line(cursor_.row).size());
}

void Document::MoveDown() {
  if (cursor_.row + 1 >= buffer_.LineCount()) return;
  cursor_.row += 1;
  cursor_.col = std::min(cursor_.col, buffer_.Line(cursor_.row).size());
}

void Document::MoveHome() { cursor_.col = 0; }

void Document::MoveEnd() { cursor_.col = buffer_.Line(cursor_.row).size(); }

void Document::MovePageUp(size_t page_size) {
  cursor_.row = (cursor_.row > page_size) ? cursor_.row - page_size : 0;
  cursor_.col = std::min(cursor_.col, buffer_.Line(cursor_.row).size());
}

void Document::MovePageDown(size_t page_size) {
  size_t last_row = buffer_.LineCount() - 1;
  cursor_.row = std::min(last_row, cursor_.row + page_size);
  cursor_.col = std::min(cursor_.col, buffer_.Line(cursor_.row).size());
}

bool Document::Undo() {
  BufferEdit edit{};
  bool ok = buffer_.Undo(&cursor_, &edit);
  if (ok && highlighter_) highlighter_->Edit(edit, buffer_.ToString());
  return ok;
}

bool Document::Redo() {
  BufferEdit edit{};
  bool ok = buffer_.Redo(&cursor_, &edit);
  if (ok && highlighter_) highlighter_->Edit(edit, buffer_.ToString());
  return ok;
}

}  // namespace puka

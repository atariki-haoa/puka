#include "editor/Buffer.hpp"

#include <algorithm>

namespace puka {

Buffer::Buffer(std::string initial_text) {
  size_t start = 0;
  while (true) {
    size_t nl = initial_text.find('\n', start);
    if (nl == std::string::npos) {
      lines_.push_back(initial_text.substr(start));
      break;
    }
    lines_.push_back(initial_text.substr(start, nl - start));
    start = nl + 1;
  }
}

size_t Buffer::LineCount() const { return lines_.size(); }

std::string_view Buffer::Line(size_t row) const { return lines_.at(row); }

std::string Buffer::ToString() const {
  std::string out;
  for (size_t i = 0; i < lines_.size(); ++i) {
    out += lines_[i];
    if (i + 1 < lines_.size()) out += '\n';
  }
  return out;
}

size_t Buffer::ByteOffset(size_t row, size_t col) const {
  size_t offset = 0;
  for (size_t i = 0; i < row; ++i) offset += lines_[i].size() + 1;
  return offset + col;
}

std::pair<size_t, size_t> Buffer::RowCol(size_t byte_offset) const {
  size_t remaining = byte_offset;
  for (size_t row = 0; row < lines_.size(); ++row) {
    size_t line_len = lines_[row].size();
    if (remaining <= line_len) return {row, remaining};
    remaining -= line_len + 1;
  }
  return {lines_.size() - 1, lines_.back().size()};
}

void Buffer::ApplyInsert(std::vector<std::string>& lines, size_t row, size_t col,
                          std::string_view text) {
  size_t first_nl = text.find('\n');
  if (first_nl == std::string_view::npos) {
    lines[row].insert(col, text);
    return;
  }

  std::string tail = lines[row].substr(col);
  lines[row].resize(col);
  lines[row] += std::string(text.substr(0, first_nl));

  std::vector<std::string> new_lines;
  size_t start = first_nl + 1;
  while (true) {
    size_t nl = text.find('\n', start);
    if (nl == std::string_view::npos) {
      new_lines.push_back(std::string(text.substr(start)));
      break;
    }
    new_lines.push_back(std::string(text.substr(start, nl - start)));
    start = nl + 1;
  }
  new_lines.back() += tail;
  lines.insert(lines.begin() + static_cast<long>(row) + 1, new_lines.begin(), new_lines.end());
}

std::string Buffer::ApplyDelete(std::vector<std::string>& lines, size_t row0, size_t col0,
                                 size_t row1, size_t col1) {
  if (row0 == row1) {
    std::string removed = lines[row0].substr(col0, col1 - col0);
    lines[row0].erase(col0, col1 - col0);
    return removed;
  }

  std::string removed = lines[row0].substr(col0);
  for (size_t r = row0 + 1; r < row1; ++r) {
    removed += '\n';
    removed += lines[r];
  }
  removed += '\n';
  removed += lines[row1].substr(0, col1);

  std::string joined = lines[row0].substr(0, col0) + lines[row1].substr(col1);
  lines.erase(lines.begin() + static_cast<long>(row0) + 1, lines.begin() + static_cast<long>(row1) + 1);
  lines[row0] = joined;
  return removed;
}

std::pair<size_t, size_t> Buffer::EndPositionAfterInsert(size_t row, size_t col,
                                                          std::string_view text) {
  size_t last_nl = text.rfind('\n');
  if (last_nl == std::string_view::npos) return {row, col + text.size()};
  size_t newline_count = static_cast<size_t>(std::count(text.begin(), text.end(), '\n'));
  return {row + newline_count, text.size() - last_nl - 1};
}

BufferEdit Buffer::MakeInsertEdit(size_t row, size_t col, size_t end_row, size_t end_col) const {
  size_t start_byte = ByteOffset(row, col);
  size_t new_end_byte = ByteOffset(end_row, end_col);
  return BufferEdit{start_byte, start_byte, new_end_byte, row, col, row, col, end_row, end_col};
}

BufferEdit Buffer::MakeDeleteEdit(size_t row0, size_t col0, size_t row1, size_t col1) const {
  size_t start_byte = ByteOffset(row0, col0);
  size_t old_end_byte = ByteOffset(row1, col1);
  return BufferEdit{start_byte, old_end_byte, start_byte, row0, col0, row1, col1, row0, col0};
}

BufferEdit Buffer::InsertText(size_t row, size_t col, std::string_view text) {
  auto [end_row, end_col] = EndPositionAfterInsert(row, col, text);
  ApplyInsert(lines_, row, col, text);
  BufferEdit edit = MakeInsertEdit(row, col, end_row, end_col);
  dirty_ = true;
  undo_stack_.push_back(
      EditRecord{EditRecord::Kind::Insert, row, col, end_row, end_col, std::string(text)});
  redo_stack_.clear();
  return edit;
}

Buffer::DeleteResult Buffer::DeleteRange(size_t row0, size_t col0, size_t row1, size_t col1) {
  BufferEdit edit = MakeDeleteEdit(row0, col0, row1, col1);
  std::string removed = ApplyDelete(lines_, row0, col0, row1, col1);
  dirty_ = true;
  undo_stack_.push_back(EditRecord{EditRecord::Kind::Delete, row0, col0, row1, col1, removed});
  redo_stack_.clear();
  return DeleteResult{removed, edit};
}

bool Buffer::Undo(Cursor* cursor_out, BufferEdit* edit_out) {
  if (undo_stack_.empty()) return false;
  EditRecord record = undo_stack_.back();
  undo_stack_.pop_back();

  if (record.kind == EditRecord::Kind::Insert) {
    BufferEdit edit = MakeDeleteEdit(record.row0, record.col0, record.row1, record.col1);
    ApplyDelete(lines_, record.row0, record.col0, record.row1, record.col1);
    if (cursor_out) *cursor_out = Cursor{record.row0, record.col0};
    if (edit_out) *edit_out = edit;
  } else {
    ApplyInsert(lines_, record.row0, record.col0, record.text);
    BufferEdit edit = MakeInsertEdit(record.row0, record.col0, record.row1, record.col1);
    if (cursor_out) *cursor_out = Cursor{record.row1, record.col1};
    if (edit_out) *edit_out = edit;
  }
  redo_stack_.push_back(record);
  dirty_ = true;
  return true;
}

bool Buffer::Redo(Cursor* cursor_out, BufferEdit* edit_out) {
  if (redo_stack_.empty()) return false;
  EditRecord record = redo_stack_.back();
  redo_stack_.pop_back();

  if (record.kind == EditRecord::Kind::Insert) {
    ApplyInsert(lines_, record.row0, record.col0, record.text);
    BufferEdit edit = MakeInsertEdit(record.row0, record.col0, record.row1, record.col1);
    if (cursor_out) *cursor_out = Cursor{record.row1, record.col1};
    if (edit_out) *edit_out = edit;
  } else {
    BufferEdit edit = MakeDeleteEdit(record.row0, record.col0, record.row1, record.col1);
    ApplyDelete(lines_, record.row0, record.col0, record.row1, record.col1);
    if (cursor_out) *cursor_out = Cursor{record.row0, record.col0};
    if (edit_out) *edit_out = edit;
  }
  undo_stack_.push_back(record);
  dirty_ = true;
  return true;
}

}  // namespace puka

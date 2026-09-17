#pragma once
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "editor/BufferEdit.hpp"
#include "editor/Cursor.hpp"

namespace puka {

// Line-based text storage. Byte-oriented (not codepoint-aware): `col` is a
// byte offset within a line, which keeps this ready to feed tree-sitter's
// byte-offset-based edits without a storage rewrite (see BufferEdit).
class Buffer {
 public:
  explicit Buffer(std::string initial_text = "");

  size_t LineCount() const;
  std::string_view Line(size_t row) const;
  std::string ToString() const;

  size_t ByteOffset(size_t row, size_t col) const;
  std::pair<size_t, size_t> RowCol(size_t byte_offset) const;

  // Inserts `text` at (row, col). `text` may contain embedded '\n's, which
  // split lines as expected. Returns the edit's byte/row/col delta.
  BufferEdit InsertText(size_t row, size_t col, std::string_view text);

  struct DeleteResult {
    std::string text;
    BufferEdit edit;
  };
  // Deletes the half-open range [(row0,col0), (row1,col1)).
  DeleteResult DeleteRange(size_t row0, size_t col0, size_t row1, size_t col1);

  // On success, writes the cursor position the edit should leave behind into
  // `cursor_out` (if non-null), the edit's delta into `edit_out` (if
  // non-null), and returns true.
  bool Undo(Cursor* cursor_out, BufferEdit* edit_out = nullptr);
  bool Redo(Cursor* cursor_out, BufferEdit* edit_out = nullptr);

  bool dirty() const { return dirty_; }
  void MarkSaved() { dirty_ = false; }

 private:
  struct EditRecord {
    enum class Kind { Insert, Delete } kind;
    size_t row0, col0, row1, col1;
    std::string text;  // inserted text (Insert) or removed text (Delete)
  };

  static void ApplyInsert(std::vector<std::string>& lines, size_t row, size_t col,
                           std::string_view text);
  static std::string ApplyDelete(std::vector<std::string>& lines, size_t row0, size_t col0,
                                  size_t row1, size_t col1);
  static std::pair<size_t, size_t> EndPositionAfterInsert(size_t row, size_t col,
                                                           std::string_view text);

  // Must be called AFTER the insert is applied (end_row may be a newly
  // created line).
  BufferEdit MakeInsertEdit(size_t row, size_t col, size_t end_row, size_t end_col) const;
  // Must be called BEFORE the delete is applied (row1/col1 may not exist
  // afterward).
  BufferEdit MakeDeleteEdit(size_t row0, size_t col0, size_t row1, size_t col1) const;

  std::vector<std::string> lines_;
  std::vector<EditRecord> undo_stack_;
  std::vector<EditRecord> redo_stack_;
  bool dirty_ = false;
};

}  // namespace puka

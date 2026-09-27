#pragma once
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "editor/Buffer.hpp"
#include "editor/Cursor.hpp"
#include "syntax/Highlighter.hpp"

namespace puka {

class Document {
 public:
  static std::optional<Document> Open(const std::filesystem::path& path);

  // Writes to a temp file in the same directory then renames over the
  // original -- atomic, so a crash mid-save never leaves a half-written file.
  bool Save();

  const std::filesystem::path& path() const { return path_; }
  std::string DisplayName() const;
  bool dirty() const { return buffer_.dirty(); }

  Buffer& buffer() { return buffer_; }
  const Buffer& buffer() const { return buffer_; }
  Cursor& cursor() { return cursor_; }
  const Cursor& cursor() const { return cursor_; }

  // Empty if this document's extension has no registered language.
  std::vector<HighlightSpan> HighlightSpans(size_t start_byte, size_t end_byte) const;

  // General insert at the cursor; text may be a single byte or a multi-byte
  // UTF-8 character. InsertChar/InsertNewline are thin wrappers over this.
  void InsertText(std::string_view text);
  void InsertChar(char c);
  void InsertNewline();
  void DeleteBackward();
  void DeleteForward();

  // Deletes the cursor's whole line, including the newline that joins it to
  // whichever line takes its place (see the .cpp for how the last line and
  // a single-line buffer -- neither has a "next" newline to consume --
  // are handled instead).
  void DeleteLine();

  // `extend_selection` is true for Shift+<key> (EditorView's raw Shift-arrow
  // handling): on the first such call after a plain move, it anchors the
  // selection at the cursor's pre-move position; on a plain move it drops any
  // existing anchor instead. The cursor itself always ends up wherever the
  // unmodified version of the move already puts it -- unlike VSCode, a plain
  // arrow with a selection active does not collapse to the selection's near/
  // far edge first, which would add real complexity for little benefit here.
  void MoveLeft(bool extend_selection = false);
  void MoveRight(bool extend_selection = false);
  void MoveUp(bool extend_selection = false);
  void MoveDown(bool extend_selection = false);
  void MoveHome(bool extend_selection = false);
  void MoveEnd(bool extend_selection = false);
  void MovePageUp(size_t page_size, bool extend_selection = false);
  void MovePageDown(size_t page_size, bool extend_selection = false);

  bool HasSelection() const { return selection_anchor_.has_value(); }
  void ClearSelection() { selection_anchor_.reset(); }
  // Normalized (start <= end by row then col) regardless of which direction
  // the selection was made in.
  std::pair<Cursor, Cursor> SelectionRange() const;
  std::string SelectedText() const;
  // What Ctrl+C/Ctrl+X act on: the selection if there is one, otherwise the
  // whole current line (with its trailing newline) -- this single helper is
  // what makes "copy current line" and "copy selection" the same feature.
  std::string SelectionOrLineText() const;
  // Deletes the active selection and collapses the cursor to its start.
  // Precondition: HasSelection().
  void DeleteSelection();
  // Ctrl+X: DeleteSelection() if there's a selection, else the existing
  // whole-line DeleteLine().
  void CutSelectionOrLine();

  bool Undo();
  bool Redo();

 private:
  Document(std::filesystem::path path, std::string content);

  // Shared by the Move* overloads above: anchors the selection at the
  // current cursor position on the first extend, or drops it on a plain
  // move. Must be called BEFORE the cursor itself moves.
  void UpdateSelectionAnchor(bool extend_selection);

  std::filesystem::path path_;
  Buffer buffer_;
  Cursor cursor_;
  std::optional<Cursor> selection_anchor_;
  std::unique_ptr<Highlighter> highlighter_;  // nullptr => unrecognized extension, no-op
};

}  // namespace puka

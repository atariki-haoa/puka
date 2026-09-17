#pragma once
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
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

  void MoveLeft();
  void MoveRight();
  void MoveUp();
  void MoveDown();
  void MoveHome();
  void MoveEnd();
  void MovePageUp(size_t page_size);
  void MovePageDown(size_t page_size);

  bool Undo();
  bool Redo();

 private:
  Document(std::filesystem::path path, std::string content);

  std::filesystem::path path_;
  Buffer buffer_;
  Cursor cursor_;
  std::unique_ptr<Highlighter> highlighter_;  // nullptr => unrecognized extension, no-op
};

}  // namespace puka

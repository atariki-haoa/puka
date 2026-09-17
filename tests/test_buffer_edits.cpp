#include "editor/Buffer.hpp"
#include "editor/Cursor.hpp"
#include "test_util.hpp"

using namespace puka;
using namespace puka::test;

namespace {

void TestInsertAndDeleteSingleLine() {
  Buffer buf("hello");
  buf.InsertText(0, 5, "!");
  Check(buf.ToString() == "hello!", "insert at end of line");
  buf.DeleteRange(0, 0, 0, 5);
  Check(buf.ToString() == "!", "delete range within a line");
}

void TestNewlineSplitsLine() {
  Buffer buf("helloworld");
  buf.InsertText(0, 5, "\n");
  Check(buf.LineCount() == 2, "newline insert produces two lines");
  Check(buf.Line(0) == "hello", "first half before split");
  Check(buf.Line(1) == "world", "second half after split");
}

void TestMultilineDeleteJoinsLines() {
  Buffer buf("foo\nbar\nbaz");
  buf.DeleteRange(0, 1, 2, 1);
  Check(buf.ToString() == "faz", "multi-line delete joins remaining fragments");
}

void TestUndoRedoRoundTrip() {
  Buffer buf("hello");
  Cursor cursor;
  buf.InsertText(0, 5, " world");
  Check(buf.ToString() == "hello world", "insert applied");
  Check(buf.Undo(&cursor), "undo reports success");
  Check(buf.ToString() == "hello", "undo restores prior content");
  Check(buf.Redo(&cursor), "redo reports success");
  Check(buf.ToString() == "hello world", "redo re-applies the edit");
}

void TestUndoRedoEmptyStacksAreNoop() {
  Buffer buf("x");
  Cursor cursor;
  Check(!buf.Undo(&cursor), "undo with empty stack returns false");
  Check(!buf.Redo(&cursor), "redo with empty stack returns false");
}

void TestByteOffsetRoundTrip() {
  Buffer buf("ab\ncd\nef");
  Check(buf.ByteOffset(1, 1) == 4, "byte offset accounts for prior lines' newlines");
  auto [row, col] = buf.RowCol(4);
  Check(row == 1 && col == 1, "row/col round-trips from byte offset");
}

}  // namespace

int main() {
  TestInsertAndDeleteSingleLine();
  TestNewlineSplitsLine();
  TestMultilineDeleteJoinsLines();
  TestUndoRedoRoundTrip();
  TestUndoRedoEmptyStacksAreNoop();
  TestByteOffsetRoundTrip();

  if (g_failures == 0) {
    std::cout << "test_buffer_edits: all tests passed\n";
    return 0;
  }
  std::cerr << "test_buffer_edits: " << g_failures << " failure(s)\n";
  return 1;
}

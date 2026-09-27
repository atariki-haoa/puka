#include <filesystem>
#include <fstream>

#include "editor/Document.hpp"
#include "test_util.hpp"

using namespace puka;
using namespace puka::test;

namespace {

std::filesystem::path WriteTempFile(const std::string& content) {
  auto path = std::filesystem::temp_directory_path() / "puka_test_document_selection.txt";
  std::ofstream(path, std::ios::binary) << content;
  return path;
}

void TestSelectionRangeNormalizesRegardlessOfDirection() {
  auto path = WriteTempFile("hello\nworld\nfoo");
  auto doc = Document::Open(path);
  Check(doc.has_value(), "document opens");

  // Forward drag: anchor before the cursor.
  doc->cursor() = {0, 0};
  doc->ClearSelection();
  doc->MoveRight(true);
  doc->MoveRight(true);
  doc->MoveRight(true);
  Check(doc->HasSelection(), "selection active after Shift+Right");
  auto [start, end] = doc->SelectionRange();
  Check(start.row == 0 && start.col == 0, "forward selection start is the anchor");
  Check(end.row == 0 && end.col == 3, "forward selection end is the cursor");
  Check(doc->SelectedText() == "hel", "forward selected text");

  // Reverse drag: anchor after the cursor -- SelectionRange must still
  // normalize to start <= end regardless of drag direction.
  doc->cursor() = {0, 5};
  doc->ClearSelection();
  doc->MoveLeft(true);
  doc->MoveLeft(true);
  auto [start2, end2] = doc->SelectionRange();
  Check(start2.row == 0 && start2.col == 3, "reverse selection still normalizes to the earlier position");
  Check(end2.row == 0 && end2.col == 5, "reverse selection still normalizes to the later position");
  Check(doc->SelectedText() == "lo", "reverse selected text");

  std::filesystem::remove(path);
}

void TestMultiRowSelectionSpansTheLineBreak() {
  auto path = WriteTempFile("hello\nworld\nfoo");
  auto doc = Document::Open(path);

  doc->cursor() = {0, 3};
  doc->ClearSelection();
  doc->MoveDown(true);
  Check(doc->SelectedText() == "lo\nwor", "selection across a line break includes the newline");

  std::filesystem::remove(path);
}

void TestPlainMoveCollapsesSelection() {
  auto path = WriteTempFile("hello world");
  auto doc = Document::Open(path);

  doc->cursor() = {0, 0};
  doc->ClearSelection();
  doc->MoveRight(true);
  doc->MoveRight(true);
  Check(doc->HasSelection(), "selection active after two Shift+Right");
  doc->MoveRight();
  Check(!doc->HasSelection(), "a plain (non-Shift) move drops the selection");

  std::filesystem::remove(path);
}

void TestSelectionOrLineTextFallsBackToCurrentLine() {
  auto path = WriteTempFile("hello\nworld");
  auto doc = Document::Open(path);

  doc->cursor() = {0, 2};
  doc->ClearSelection();
  Check(doc->SelectionOrLineText() == "hello\n", "no selection -> whole current line plus newline");

  doc->MoveRight(true);
  doc->MoveRight(true);
  Check(doc->SelectionOrLineText() == "ll", "with a selection -> just the selected text");

  std::filesystem::remove(path);
}

void TestDeleteSelectionCollapsesCursorAndClearsAnchor() {
  auto path = WriteTempFile("hello world");
  auto doc = Document::Open(path);

  doc->cursor() = {0, 0};
  doc->ClearSelection();
  for (int i = 0; i < 5; ++i) doc->MoveRight(true);  // selects "hello"
  doc->DeleteSelection();
  Check(doc->buffer().ToString() == " world", "DeleteSelection removes the selected range");
  Check(doc->cursor().row == 0 && doc->cursor().col == 0, "cursor collapses to the selection start");
  Check(!doc->HasSelection(), "selection is cleared after deleting it");

  std::filesystem::remove(path);
}

void TestTypingOverSelectionReplacesIt() {
  auto path = WriteTempFile("hello world");
  auto doc = Document::Open(path);

  doc->cursor() = {0, 0};
  doc->ClearSelection();
  for (int i = 0; i < 5; ++i) doc->MoveRight(true);  // selects "hello"
  doc->InsertText("bye");
  Check(doc->buffer().ToString() == "bye world", "typing over an active selection replaces it");
  Check(!doc->HasSelection(), "selection is cleared after the replacing insert");

  std::filesystem::remove(path);
}

void TestBackspaceOverSelectionDeletesJustTheSelection() {
  auto path = WriteTempFile("hello world");
  auto doc = Document::Open(path);

  doc->cursor() = {0, 0};
  doc->ClearSelection();
  for (int i = 0; i < 5; ++i) doc->MoveRight(true);  // selects "hello"
  doc->DeleteBackward();
  Check(doc->buffer().ToString() == " world",
        "Backspace over a selection deletes the selection, not one character");

  std::filesystem::remove(path);
}

void TestCutSelectionOrLineFallsBackToWholeLine() {
  auto path = WriteTempFile("hello\nworld");
  auto doc = Document::Open(path);

  doc->cursor() = {0, 2};
  doc->ClearSelection();
  doc->CutSelectionOrLine();
  Check(doc->buffer().ToString() == "world", "no selection -> Ctrl+X cuts the whole current line");

  std::filesystem::remove(path);
}

}  // namespace

int main() {
  TestSelectionRangeNormalizesRegardlessOfDirection();
  TestMultiRowSelectionSpansTheLineBreak();
  TestPlainMoveCollapsesSelection();
  TestSelectionOrLineTextFallsBackToCurrentLine();
  TestDeleteSelectionCollapsesCursorAndClearsAnchor();
  TestTypingOverSelectionReplacesIt();
  TestBackspaceOverSelectionDeletesJustTheSelection();
  TestCutSelectionOrLineFallsBackToWholeLine();

  if (g_failures == 0) {
    std::cout << "test_document_selection: all tests passed\n";
    return 0;
  }
  std::cerr << "test_document_selection: " << g_failures << " failure(s)\n";
  return 1;
}

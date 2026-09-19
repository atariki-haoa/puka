#pragma once
#include <filesystem>
#include <vector>

#include "editor/Document.hpp"

namespace puka {

// Owns all open documents (tabs). Returned Document* pointers must not be
// cached across calls that can add/remove documents (OpenFile/CloseActive) --
// callers should re-fetch via Active() instead.
class DocumentManager {
 public:
  Document* OpenFile(const std::filesystem::path& path);
  void CloseActive();
  void NextTab();
  void PrevTab();
  // No-op if `index` is out of range -- callers (mouse clicks, Alt+1..9)
  // derive `index` from data already clamped to the open tab count, so this
  // guard is only ever hit if a tab closed between the click/keypress and
  // dispatch.
  void SetActiveIndex(size_t index);

  Document* Active();
  const std::vector<Document>& documents() const { return docs_; }
  size_t active_index() const { return active_; }
  bool empty() const { return docs_.empty(); }

 private:
  std::vector<Document> docs_;
  size_t active_ = 0;
};

}  // namespace puka

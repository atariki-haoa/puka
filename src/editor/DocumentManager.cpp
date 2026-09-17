#include "editor/DocumentManager.hpp"

namespace puka {

Document* DocumentManager::OpenFile(const std::filesystem::path& path) {
  std::error_code ec;
  auto canonical = std::filesystem::weakly_canonical(path, ec);

  for (size_t i = 0; i < docs_.size(); ++i) {
    std::error_code ec2;
    if (std::filesystem::weakly_canonical(docs_[i].path(), ec2) == canonical) {
      active_ = i;
      return &docs_[i];
    }
  }

  auto opened = Document::Open(path);
  if (!opened) return nullptr;
  docs_.push_back(std::move(*opened));
  active_ = docs_.size() - 1;
  return &docs_[active_];
}

void DocumentManager::CloseActive() {
  if (docs_.empty()) return;
  docs_.erase(docs_.begin() + static_cast<long>(active_));
  if (!docs_.empty() && active_ >= docs_.size()) active_ = docs_.size() - 1;
}

void DocumentManager::NextTab() {
  if (docs_.empty()) return;
  active_ = (active_ + 1) % docs_.size();
}

void DocumentManager::PrevTab() {
  if (docs_.empty()) return;
  active_ = (active_ + docs_.size() - 1) % docs_.size();
}

Document* DocumentManager::Active() {
  if (docs_.empty()) return nullptr;
  return &docs_[active_];
}

}  // namespace puka

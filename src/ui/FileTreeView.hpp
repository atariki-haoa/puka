#pragma once
#include <filesystem>
#include <functional>
#include <unordered_map>
#include <vector>

#include <ftxui/component/component_base.hpp>

#include "fs/FileTree.hpp"
#include "git/GitService.hpp"

namespace puka {

class FileTreeView : public ftxui::ComponentBase {
 public:
  // `git_status` may be nullptr (no badges shown); when non-null, its
  // pointee is read fresh on every render, so the caller can update its
  // contents in place (e.g. after Application::RefreshGitStatus()) without
  // needing to push anything to this view.
  FileTreeView(std::filesystem::path root,
               std::function<void(const std::filesystem::path&)> on_open,
               const std::unordered_map<std::filesystem::path, GitFileStatus>* git_status = nullptr);

  ftxui::Element OnRender() override;
  bool OnEvent(ftxui::Event event) override;
  bool Focusable() const override { return true; }

 private:
  void RefreshVisible();

  FileTree tree_;
  std::function<void(const std::filesystem::path&)> on_open_;
  const std::unordered_map<std::filesystem::path, GitFileStatus>* git_status_;
  std::vector<FileTree::VisibleRow> visible_;
  int selected_ = 0;
};

}  // namespace puka

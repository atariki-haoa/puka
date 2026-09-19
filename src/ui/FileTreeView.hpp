#pragma once
#include <filesystem>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include <ftxui/component/component_base.hpp>
#include <ftxui/screen/box.hpp>

#include "fs/FileTree.hpp"
#include "git/GitService.hpp"

namespace puka {

class FileTreeView : public ftxui::ComponentBase {
 public:
  // `git_status` may be nullptr (no badges shown); when non-null, its
  // pointee is read fresh on every render, so the caller can update its
  // contents in place (e.g. after Application::RefreshGitStatus()) without
  // needing to push anything to this view. `ignored_paths` works the same
  // way: nullptr means no dimming, non-null is read fresh on every render
  // (see GitRepoStatus::ignored_paths for what it holds).
  FileTreeView(std::filesystem::path root,
               std::function<void(const std::filesystem::path&)> on_open,
               const std::unordered_map<std::filesystem::path, GitFileStatus>* git_status = nullptr,
               const std::vector<std::filesystem::path>* ignored_paths = nullptr);

  ftxui::Element OnRender() override;
  bool OnEvent(ftxui::Event event) override;
  bool Focusable() const override { return true; }

  // True while the "new file" name prompt is open -- Application checks this
  // to suppress global commands the same way it does for the diff view/
  // shortcuts popup, so e.g. Esc cancels the prompt instead of toggling pane
  // focus. See CreateFile()'s doc comment for where the file actually lands.
  bool CreatingFile() const { return creating_file_; }

  // True while the delete confirmation prompt is open -- same reasoning and
  // same Application-side suppression as CreatingFile() above.
  bool DeletingFile() const { return deleting_file_; }

 private:
  void RefreshVisible();
  void StartCreateFile();
  void CreateFile();
  void StartDeleteFile();
  void DeleteFile();
  ftxui::Element RenderTree();
  FileTreeNode* FindVisibleNodeByPath(const std::filesystem::path& path);
  // Opens the file, or toggles the folder, at `selected_` -- shared by the
  // Return key and a mouse click on the row, which mean the same thing.
  void ActivateSelected();

  // Declared before tree_ so the constructor can copy it before moving the
  // same value into FileTree's constructor (member init order follows
  // declaration order, not the initializer list).
  std::filesystem::path root_;

  FileTree tree_;
  std::function<void(const std::filesystem::path&)> on_open_;
  const std::unordered_map<std::filesystem::path, GitFileStatus>* git_status_;
  const std::vector<std::filesystem::path>* ignored_paths_;
  std::vector<FileTree::VisibleRow> visible_;
  int selected_ = 0;
  // One box per visible row, captured by RenderTree's reflect() so OnEvent
  // can hit-test a mouse click against it -- rebuilt every render, same as
  // visible_ itself.
  std::vector<ftxui::Box> row_boxes_;

  bool creating_file_ = false;
  std::string new_file_name_;
  FileTreeNode* new_file_dir_node_ = nullptr;

  bool deleting_file_ = false;
  FileTreeNode* delete_target_ = nullptr;
};

}  // namespace puka

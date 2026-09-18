#pragma once
#include <filesystem>
#include <functional>
#include <vector>

#include <ftxui/component/component_base.hpp>

#include "git/GitService.hpp"
#include "ui/ScmTree.hpp"

namespace puka {

// List shows every changed file as one flat row (full path from the
// workspace root); Tree groups them into their folder structure, like
// VSCode's Source Control panel toggle. Defaults to List, matching current
// behavior -- press `t` while Source Control is focused to switch.
enum class ScmViewMode { List, Tree };

// Enter opens a file's diff against HEAD; Shift+Enter (or the `o` fallback,
// since Shift+Enter isn't reliably distinguishable from plain Enter in
// every terminal -- see the raw chord comment in OnEventList/OnEventTree)
// opens the file directly, bypassing the diff view entirely.
enum class ScmOpenMode { Diff, File };

class SourceControlView : public ftxui::ComponentBase {
 public:
  SourceControlView(std::filesystem::path root,
                     std::function<void(const std::filesystem::path&, ScmOpenMode)> on_open,
                     std::function<void()> on_refresh_requested);

  ftxui::Element OnRender() override;
  bool OnEvent(ftxui::Event event) override;
  bool Focusable() const override { return true; }

  // Application is the sole caller of GetRepoStatus(); this is purely a
  // display sink so FileTreeView can share the same computed result
  // without it being computed twice per refresh.
  void SetStatus(GitRepoStatus status);

 private:
  ftxui::Element RenderList();
  ftxui::Element RenderTree();
  bool OnEventList(ftxui::Event event);
  bool OnEventTree(ftxui::Event event);
  void RefreshTreeVisible();
  const GitFileStatus* SelectedFileList() const;
  ScmTree& TreeForVisibleIndex(int index);

  std::filesystem::path root_;
  std::function<void(const std::filesystem::path&, ScmOpenMode)> on_open_;
  std::function<void()> on_refresh_requested_;
  GitRepoStatus status_;
  ScmViewMode mode_ = ScmViewMode::List;
  int list_selected_ = 0;

  // status_.files split into the two groups VSCode's Source Control panel
  // shows as separate sections -- a file with both a staged change and
  // further unstaged edits on top appears in both (see GitFileStatus's own
  // doc comment: staged/unstaged are independent). Rebuilt wholesale in
  // SetStatus() alongside status_ itself.
  std::vector<GitFileStatus> staged_;
  std::vector<GitFileStatus> unstaged_;

  // Two independent trees (rather than one tree tagged by section) so each
  // section keeps its own collapsed-folder state, same as VSCode.
  ScmTree tree_staged_;
  ScmTree tree_unstaged_;
  std::vector<ScmTree::VisibleRow> tree_visible_;  // tree_staged_'s rows, then tree_unstaged_'s
  size_t tree_staged_row_count_ = 0;                // boundary within tree_visible_
  int tree_selected_ = 0;
};

}  // namespace puka

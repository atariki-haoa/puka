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

class SourceControlView : public ftxui::ComponentBase {
 public:
  SourceControlView(std::filesystem::path root,
                     std::function<void(const std::filesystem::path&)> on_open,
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

  std::filesystem::path root_;
  std::function<void(const std::filesystem::path&)> on_open_;
  std::function<void()> on_refresh_requested_;
  GitRepoStatus status_;
  ScmViewMode mode_ = ScmViewMode::List;
  int list_selected_ = 0;

  ScmTree tree_;
  std::vector<ScmTree::VisibleRow> tree_visible_;
  int tree_selected_ = 0;
};

}  // namespace puka

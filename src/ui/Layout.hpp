#pragma once
#include <string>

#include <ftxui/component/component_base.hpp>

#include "git/GitService.hpp"

namespace puka {

// Wraps a ResizableSplitLeft(sidebar, editor) component so the sidebar's
// rendered pane can be hidden (Ctrl+B) while keeping its internal state --
// scroll position, expanded folders -- intact for when it's shown again.
// Also renders a status bar footer, spanning the full width below the split.
class Layout : public ftxui::ComponentBase {
 public:
  Layout(ftxui::Component sidebar, ftxui::Component editor, int* sidebar_width,
         std::string status_hint);

  ftxui::Element OnRender() override;

  void SetSidebarVisible(bool visible) { sidebar_visible_ = visible; }
  bool SidebarVisible() const { return sidebar_visible_; }

  void SetGitStatus(GitRepoStatus git_status) { git_status_ = std::move(git_status); }

 private:
  ftxui::Component editor_;
  std::string status_hint_;
  bool sidebar_visible_ = true;
  GitRepoStatus git_status_;
};

}  // namespace puka

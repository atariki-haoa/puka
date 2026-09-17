#pragma once
#include <ftxui/component/component_base.hpp>

namespace puka {

// Wraps a ResizableSplitLeft(sidebar, editor) component so the sidebar's
// rendered pane can be hidden (Ctrl+B) while keeping its internal state --
// scroll position, expanded folders -- intact for when it's shown again.
class Layout : public ftxui::ComponentBase {
 public:
  Layout(ftxui::Component sidebar, ftxui::Component editor, int* sidebar_width);

  ftxui::Element OnRender() override;

  void SetSidebarVisible(bool visible) { sidebar_visible_ = visible; }
  bool SidebarVisible() const { return sidebar_visible_; }

 private:
  ftxui::Component editor_;
  bool sidebar_visible_ = true;
};

}  // namespace puka

#pragma once
#include <ftxui/component/component_base.hpp>

namespace puka {

enum class SidebarView { Explorer, Search, SourceControl };

// Simple one-line Explorer|Search|Source-Control switcher. A fancier
// vertical icon rail (closer to VSCode's activity bar) is deferred as
// non-blocking visual polish -- this keeps the same active_ state machine.
class Sidebar : public ftxui::ComponentBase {
 public:
  Sidebar(ftxui::Component explorer, ftxui::Component search, ftxui::Component source_control);

  ftxui::Element OnRender() override;
  bool OnEvent(ftxui::Event event) override;

  void SetActiveView(SidebarView view) { active_ = view; }
  SidebarView ActiveView() const { return active_; }

  // Moves to the next (+1) or previous (-1) view, wrapping around.
  void CycleView(int direction);

 private:
  SidebarView active_ = SidebarView::Explorer;
};

}  // namespace puka

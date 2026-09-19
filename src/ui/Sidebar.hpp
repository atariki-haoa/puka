#pragma once
#include <array>
#include <functional>

#include <ftxui/component/component_base.hpp>
#include <ftxui/screen/box.hpp>

namespace puka {

enum class SidebarView { Explorer, Search, SourceControl };

// Simple one-line Explorer|Search|Source-Control switcher. A fancier
// vertical icon rail (closer to VSCode's activity bar) is deferred as
// non-blocking visual polish -- this keeps the same active_ state machine.
class Sidebar : public ftxui::ComponentBase {
 public:
  // `on_view_clicked` fires only for a mouse click on the switcher (not for
  // SetActiveView/CycleView, which Application already drives explicitly and
  // pairs with its own side effects, e.g. refreshing git status on Alt+G) --
  // same constructor-injected-callback pattern as Explorer/Search/Source
  // Control's own on_open callbacks, see CLAUDE.md's UI composition section.
  Sidebar(ftxui::Component explorer, ftxui::Component search, ftxui::Component source_control,
          std::function<void(SidebarView)> on_view_clicked);

  ftxui::Element OnRender() override;
  bool OnEvent(ftxui::Event event) override;

  void SetActiveView(SidebarView view) { active_ = view; }
  SidebarView ActiveView() const { return active_; }

  // Moves to the next (+1) or previous (-1) view, wrapping around.
  void CycleView(int direction);

 private:
  SidebarView active_ = SidebarView::Explorer;
  std::function<void(SidebarView)> on_view_clicked_;
  // One box per switcher icon (Explorer/Search/Source Control, in that
  // order), captured by OnRender's reflect() for OnEvent's mouse hit-test.
  std::array<ftxui::Box, 3> switcher_boxes_;
};

}  // namespace puka

#include "ui/Sidebar.hpp"

#include <utility>

#include <ftxui/component/event.hpp>
#include <ftxui/component/mouse.hpp>
#include <ftxui/dom/elements.hpp>

#include "ui/Icons.hpp"

namespace puka {
using namespace ftxui;

namespace {

std::string_view Glyph(SidebarView view) {
  switch (view) {
    case SidebarView::Explorer: return Icons::ExplorerGlyph();
    case SidebarView::Search: return Icons::SearchGlyph();
    case SidebarView::SourceControl: return Icons::SourceControlGlyph();
  }
  return "";
}

// See Icons::ExplorerColor()'s doc comment: selected=false is the panel's
// accent hue (an inactive tab's glyph color); selected=true is the
// foreground that stays legible once that same accent is used as the
// active tab's bgcolor() fill.
Color Accent(SidebarView view, bool selected) {
  switch (view) {
    case SidebarView::Explorer: return Icons::ExplorerColor(selected);
    case SidebarView::Search: return Icons::SearchColor(selected);
    case SidebarView::SourceControl: return Icons::SourceControlColor(selected);
  }
  return Color::Default;
}

}  // namespace

Sidebar::Sidebar(Component explorer, Component search, Component source_control,
                  std::function<void(SidebarView)> on_view_clicked)
    : on_view_clicked_(std::move(on_view_clicked)) {
  Add(std::move(explorer));
  Add(std::move(search));
  Add(std::move(source_control));
}

Element Sidebar::OnRender() {
  bool focused = Focused();

  Elements tabs;
  for (int i = 0; i < 3; ++i) {
    auto view = static_cast<SidebarView>(i);
    bool selected = (view == active_);
    Element label = text("  " + std::string(Glyph(view)) + "  ");
    // Each panel keeps its own accent at all times (so the three stay
    // easy to tell apart by color alone); the active one additionally
    // gets its accent as a bgcolor() fill, with a foreground picked for
    // contrast against that specific fill -- not the file tree's plain
    // inverted/bold, which would erase the per-panel color identity on
    // selection.
    label = selected ? label | bold | color(Accent(view, true)) | bgcolor(Accent(view, false))
                      : label | color(Accent(view, false));
    label = label | reflect(switcher_boxes_[static_cast<size_t>(i)]);
    tabs.push_back(label);
  }
  Element switcher = hbox(std::move(tabs));

  Element content = ChildAt(static_cast<size_t>(active_))->Render();

  // Same instant color flip as EditorView's top divider -- whichever pane's
  // divider is cyan is the one that will receive your next keystroke.
  Element sep = separator() | color(focused ? Color::Cyan : Color::GrayDark);

  return vbox({switcher, sep, content | flex});
}

bool Sidebar::OnEvent(Event event) {
  if (event.is_mouse() && event.mouse().button == Mouse::Left &&
      event.mouse().motion == Mouse::Pressed) {
    for (size_t i = 0; i < switcher_boxes_.size(); ++i) {
      if (!switcher_boxes_[i].Contain(event.mouse().x, event.mouse().y)) continue;
      active_ = static_cast<SidebarView>(i);
      TakeFocus();
      if (on_view_clicked_) on_view_clicked_(active_);
      return true;
    }
  }
  return ChildAt(static_cast<size_t>(active_))->OnEvent(event);
}

void Sidebar::CycleView(int direction) {
  constexpr int kViewCount = 3;
  int next = (static_cast<int>(active_) + direction + kViewCount) % kViewCount;
  active_ = static_cast<SidebarView>(next);
}

}  // namespace puka

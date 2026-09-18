#include "ui/Sidebar.hpp"

#include <ftxui/component/event.hpp>
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

}  // namespace

Sidebar::Sidebar(Component explorer, Component search, Component source_control) {
  Add(std::move(explorer));
  Add(std::move(search));
  Add(std::move(source_control));
}

Element Sidebar::OnRender() {
  bool focused = Focused();

  Elements tabs;
  for (int i = 0; i < 3; ++i) {
    auto view = static_cast<SidebarView>(i);
    Element label = text("  " + std::string(Glyph(view)) + "  ");
    if (view == active_) label = label | inverted | bold;
    tabs.push_back(label);
  }
  Element switcher = hbox(std::move(tabs));
  if (focused) switcher = switcher | color(Color::Cyan);

  Element content = ChildAt(static_cast<size_t>(active_))->Render();

  // Same instant color flip as EditorView's top divider -- whichever pane's
  // divider is cyan is the one that will receive your next keystroke.
  Element sep = separator() | color(focused ? Color::Cyan : Color::GrayDark);

  return vbox({switcher, sep, content | flex});
}

bool Sidebar::OnEvent(Event event) {
  return ChildAt(static_cast<size_t>(active_))->OnEvent(event);
}

void Sidebar::CycleView(int direction) {
  constexpr int kViewCount = 3;
  int next = (static_cast<int>(active_) + direction + kViewCount) % kViewCount;
  active_ = static_cast<SidebarView>(next);
}

}  // namespace puka

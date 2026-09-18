#include "ui/Layout.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>

#include "ui/StatusBar.hpp"

namespace puka {
using namespace ftxui;

Layout::Layout(Component sidebar, Component editor, int* sidebar_width, std::string status_hint)
    : editor_(editor), status_hint_(std::move(status_hint)) {
  Add(ResizableSplitLeft(std::move(sidebar), std::move(editor), sidebar_width));
}

Element Layout::OnRender() {
  Element content = sidebar_visible_ ? ChildAt(0)->Render() : editor_->Render();
  return vbox({content | flex, separator(), RenderStatusBar(status_hint_, git_status_)});
}

}  // namespace puka

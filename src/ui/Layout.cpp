#include "ui/Layout.hpp"

#include <ftxui/component/component.hpp>

namespace puka {
using namespace ftxui;

Layout::Layout(Component sidebar, Component editor, int* sidebar_width) : editor_(editor) {
  Add(ResizableSplitLeft(std::move(sidebar), std::move(editor), sidebar_width));
}

Element Layout::OnRender() {
  return sidebar_visible_ ? ChildAt(0)->Render() : editor_->Render();
}

}  // namespace puka

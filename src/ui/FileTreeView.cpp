#include "ui/FileTreeView.hpp"

#include <algorithm>

#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include "ui/GitStatusBadge.hpp"
#include "ui/Icons.hpp"

namespace puka {
using namespace ftxui;

FileTreeView::FileTreeView(std::filesystem::path root,
                            std::function<void(const std::filesystem::path&)> on_open,
                            const std::unordered_map<std::filesystem::path, GitFileStatus>* git_status)
    : tree_(std::move(root)), on_open_(std::move(on_open)), git_status_(git_status) {
  RefreshVisible();
}

void FileTreeView::RefreshVisible() {
  visible_ = tree_.VisibleRows();
  selected_ = visible_.empty() ? 0 : std::clamp(selected_, 0, static_cast<int>(visible_.size()) - 1);
}

Element FileTreeView::OnRender() {
  if (visible_.empty()) {
    return vbox({filler(), hcenter(text("(empty folder)") | dim), filler()});
  }

  Elements rows;
  for (int i = 0; i < static_cast<int>(visible_.size()); ++i) {
    const auto& row = visible_[i];
    std::string indent(static_cast<size_t>(row.depth) * 2, ' ');
    std::string_view glyph = row.node->is_directory
                                  ? Icons::FolderGlyph(row.node->expanded)
                                  : Icons::FileGlyph(row.node->path.extension().string());
    std::string label = indent + std::string(glyph) + " " + row.node->name;
    Element line = text(label);

    if (git_status_) {
      auto it = git_status_->find(row.node->path);
      if (it != git_status_->end()) {
        GitDeltaType primary = PrimaryDelta(it->second);
        Element badge = text(std::string(1, BadgeChar(primary))) | color(BadgeColor(primary));
        line = hbox({line, filler(), badge, text(" ")});
      }
    }

    if (i == selected_) line = line | inverted;
    rows.push_back(line);
  }
  return vbox(std::move(rows)) | focusPosition(0, selected_) | frame | flex;
}

bool FileTreeView::OnEvent(Event event) {
  if (visible_.empty()) return false;

  if (event == Event::ArrowUp) {
    selected_ = std::max(0, selected_ - 1);
    return true;
  }
  if (event == Event::ArrowDown) {
    selected_ = std::min(static_cast<int>(visible_.size()) - 1, selected_ + 1);
    return true;
  }

  auto* node = visible_[static_cast<size_t>(selected_)].node;
  if (event == Event::Return || event == Event::ArrowRight) {
    if (node->is_directory) {
      tree_.ToggleExpanded(*node);
      RefreshVisible();
    } else if (on_open_) {
      on_open_(node->path);
    }
    return true;
  }
  if (event == Event::ArrowLeft) {
    if (node->is_directory && node->expanded) {
      tree_.ToggleExpanded(*node);
      RefreshVisible();
    }
    return true;
  }
  return false;
}

}  // namespace puka

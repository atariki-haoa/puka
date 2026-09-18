#include "ui/FileTreeView.hpp"

#include <algorithm>

#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include "git/GitService.hpp"
#include "ui/GitStatusBadge.hpp"
#include "ui/Icons.hpp"

namespace puka {
using namespace ftxui;

namespace {

// gitignored entries render in a flat neutral grey -- overriding the
// per-extension icon color -- so they read as "excluded" at a glance,
// matching the user-facing ask rather than just dimming whatever hue the
// extension would otherwise get. Darker variant for selected rows, same
// reasoning as Icons::FileColor's `selected` darkening.
const Color kIgnoredColor = Color::RGB(0x75, 0x75, 0x75);
const Color kIgnoredColorSelected = Color::RGB(0x34, 0x34, 0x34);

}  // namespace

FileTreeView::FileTreeView(std::filesystem::path root,
                            std::function<void(const std::filesystem::path&)> on_open,
                            const std::unordered_map<std::filesystem::path, GitFileStatus>* git_status,
                            const std::vector<std::filesystem::path>* ignored_paths)
    : tree_(std::move(root)),
      on_open_(std::move(on_open)),
      git_status_(git_status),
      ignored_paths_(ignored_paths) {
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
    bool selected = (i == selected_);
    bool ignored = ignored_paths_ && IsPathIgnored(row.node->path, *ignored_paths_);
    Color glyph_color = ignored ? (selected ? kIgnoredColorSelected : kIgnoredColor)
                         : row.node->is_directory
                             ? Icons::FolderColor(selected)
                             : Icons::FileColor(row.node->path.extension().string(), selected);
    // Selection wins over the ignored-grey: on the cursor's light-grey
    // background the label needs to be black to stay legible, same as any
    // other filename there.
    Color name_color = selected ? Color::Black : (ignored ? kIgnoredColor : Color::Default);
    Element line = hbox({text(indent), text(std::string(glyph)) | color(glyph_color),
                          text(" " + row.node->name) | color(name_color)});

    if (git_status_) {
      auto it = git_status_->find(row.node->path);
      if (it != git_status_->end()) {
        GitDeltaType primary = PrimaryDelta(it->second);
        Element badge = text(std::string(1, BadgeChar(primary))) | color(BadgeColor(primary));
        line = hbox({line, filler(), badge, text(" ")});
      }
    }

    // Not `| inverted`: that swaps each cell's own fg/bg individually, so the
    // icon's fg color (and the git badge's) would show through as a colored
    // background instead of a uniform selection highlight. An explicit
    // bgcolor keeps every glyph's color as foreground-on-grey instead.
    if (selected) line = line | bgcolor(Color::GrayLight);
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
  if (event == Event::Return) {
    if (node->is_directory) {
      tree_.ToggleExpanded(*node);
      RefreshVisible();
    } else if (on_open_) {
      on_open_(node->path);
    }
    return true;
  }
  if (event == Event::ArrowRight) {
    if (node->is_directory && !node->expanded) {
      tree_.ToggleExpanded(*node);
      RefreshVisible();
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

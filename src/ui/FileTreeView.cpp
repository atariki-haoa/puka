#include "ui/FileTreeView.hpp"

#include <algorithm>
#include <fstream>

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
    : root_(root),
      tree_(std::move(root)),
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
  Element body = RenderTree();
  if (creating_file_) {
    Element prompt = hbox({text(" New file: ") | bold, text(new_file_name_) | underlined, text("_")});
    return vbox({prompt, separator(), body | flex});
  }
  if (deleting_file_) {
    std::string kind = delete_target_->is_directory ? "folder" : "file";
    Element prompt = hbox({text(" Delete " + kind + " '" + delete_target_->name + "'? ") | bold,
                            text("(y/n)") | dim});
    return vbox({prompt, separator(), body | flex});
  }
  return body;
}

Element FileTreeView::RenderTree() {
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
  if (creating_file_) {
    if (event == Event::Escape) {
      creating_file_ = false;
      return true;
    }
    if (event == Event::Return) {
      if (!new_file_name_.empty()) CreateFile();
      creating_file_ = false;
      return true;
    }
    if (event == Event::Backspace) {
      if (!new_file_name_.empty()) new_file_name_.pop_back();
      return true;
    }
    if (event.is_character()) {
      new_file_name_ += event.character();
      return true;
    }
    return true;  // swallow everything else while the prompt is open
  }

  if (deleting_file_) {
    if (event == Event::Character("y") || event == Event::Character("Y")) {
      DeleteFile();
      deleting_file_ = false;
      return true;
    }
    if (event == Event::Escape || event == Event::Character("n") || event == Event::Character("N")) {
      deleting_file_ = false;
      delete_target_ = nullptr;
      return true;
    }
    return true;  // swallow everything else while the prompt is open
  }

  // Local to this view (like the SCM view's own F5/t), not a global
  // CommandRegistry binding -- only meaningful while Explorer is focused.
  // Works even on an empty folder (StartCreateFile falls back to the
  // workspace root when nothing is selected).
  if (event == Event::Character("n") || event == Event::Character("N")) {
    StartCreateFile();
    return true;
  }

  // Deletes whatever is currently selected -- no-op (StartDeleteFile
  // returns without opening the prompt) when the tree is empty, unlike "n"
  // above which can still fall back to the workspace root.
  if (event == Event::Character("d") || event == Event::Character("D")) {
    StartDeleteFile();
    return true;
  }

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

FileTreeNode* FileTreeView::FindVisibleNodeByPath(const std::filesystem::path& path) {
  for (const auto& row : visible_) {
    if (row.node->path == path) return row.node;
  }
  return nullptr;
}

void FileTreeView::StartCreateFile() {
  // Default target: the workspace root, covering both "nothing selected"
  // and "tree is empty" -- Root() is never itself a row in visible_.
  new_file_dir_node_ = &tree_.Root();
  if (!visible_.empty()) {
    auto* node = visible_[static_cast<size_t>(selected_)].node;
    if (node->is_directory) {
      new_file_dir_node_ = node;
    } else if (auto* parent = FindVisibleNodeByPath(node->path.parent_path())) {
      // A visible file's parent directory is always itself visible and
      // expanded -- that's the only way the file could be showing at all.
      new_file_dir_node_ = parent;
    }
  }
  creating_file_ = true;
  new_file_name_.clear();
}

// Touch-creates an empty file in new_file_dir_node_'s directory, refreshes
// just that directory's listing (expanding it if needed so the new file is
// visible), then opens it like any other Explorer click. Silently no-ops on
// a name that already exists or that Buffer can't be written to (e.g. a
// name containing '/' whose parent doesn't exist) -- same "fail quiet, not
// crash" posture as the rest of this read-mostly view.
void FileTreeView::CreateFile() {
  std::filesystem::path new_path = new_file_dir_node_->path / new_file_name_;
  std::error_code ec;
  if (!std::filesystem::exists(new_path, ec)) {
    std::ofstream(new_path, std::ios::binary).close();
  }

  new_file_dir_node_->expanded = true;
  tree_.RefreshChildren(*new_file_dir_node_);
  RefreshVisible();

  for (int i = 0; i < static_cast<int>(visible_.size()); ++i) {
    if (visible_[i].node->path == new_path) {
      selected_ = i;
      break;
    }
  }
  if (on_open_) on_open_(new_path);
}

void FileTreeView::StartDeleteFile() {
  if (visible_.empty()) return;
  delete_target_ = visible_[static_cast<size_t>(selected_)].node;
  deleting_file_ = true;
}

// Removes delete_target_ from disk -- remove_all so a directory goes
// recursively, which also covers the plain-file case identically to
// remove() -- then refreshes just its parent directory's listing, the same
// narrow-refresh approach CreateFile() uses. Silently no-ops on a failed
// removal (e.g. permissions): same fail-quiet posture as CreateFile().
void FileTreeView::DeleteFile() {
  std::filesystem::path deleted_path = delete_target_->path;
  std::error_code ec;
  std::filesystem::remove_all(deleted_path, ec);
  delete_target_ = nullptr;

  FileTreeNode* parent = FindVisibleNodeByPath(deleted_path.parent_path());
  if (!parent) parent = &tree_.Root();
  tree_.RefreshChildren(*parent);
  RefreshVisible();
}

}  // namespace puka

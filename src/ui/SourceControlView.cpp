#include "ui/SourceControlView.hpp"

#include <algorithm>
#include <system_error>

#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include "ui/GitStatusBadge.hpp"
#include "ui/Icons.hpp"

namespace puka {
using namespace ftxui;

namespace {

Element FileBadge(const GitFileStatus& f) {
  std::string badge = (f.unstaged == GitDeltaType::Untracked)
                           ? "??"
                           : std::string(1, BadgeChar(f.staged)) + BadgeChar(f.unstaged);
  return text(badge) | color(BadgeColor(PrimaryDelta(f)));
}

}  // namespace

SourceControlView::SourceControlView(std::filesystem::path root,
                                      std::function<void(const std::filesystem::path&)> on_open,
                                      std::function<void()> on_refresh_requested)
    : root_(std::move(root)),
      on_open_(std::move(on_open)),
      on_refresh_requested_(std::move(on_refresh_requested)) {}

void SourceControlView::SetStatus(GitRepoStatus status) {
  status_ = std::move(status);
  list_selected_ = status_.files.empty()
                        ? 0
                        : std::clamp(list_selected_, 0, static_cast<int>(status_.files.size()) - 1);
  tree_.SetFiles(status_.files, root_);
  RefreshTreeVisible();
}

void SourceControlView::RefreshTreeVisible() {
  tree_visible_ = tree_.VisibleRows();
  tree_selected_ = tree_visible_.empty()
                        ? 0
                        : std::clamp(tree_selected_, 0, static_cast<int>(tree_visible_.size()) - 1);
}

Element SourceControlView::OnRender() {
  if (!status_.is_repo) {
    return vbox({filler(), hcenter(text("Not a git repository") | dim), filler()});
  }

  std::string branch_label = (status_.detached ? "(detached) " : "") + status_.branch;
  std::string mode_label = mode_ == ScmViewMode::Tree ? "Tree" : "List";
  Element header = hbox(
      {text(branch_label) | bold | color(Color::Cyan), filler(), text(mode_label) | dim});

  if (status_.files.empty()) {
    return vbox({header, separator(), filler(), hcenter(text("No changes") | dim), filler()});
  }

  Element body = mode_ == ScmViewMode::Tree ? RenderTree() : RenderList();
  return vbox({header, separator(), body});
}

Element SourceControlView::RenderList() {
  Elements rows;
  for (int i = 0; i < static_cast<int>(status_.files.size()); ++i) {
    const auto& f = status_.files[i];
    // Two-column porcelain-style badge (staged, unstaged) so a file with
    // both a staged change and further unstaged edits on top (e.g. "MM")
    // is represented as one row, not split across separate sections.
    std::error_code ec;
    auto rel = std::filesystem::relative(f.path, root_, ec);
    std::string path_str = ec ? f.path.string() : rel.string();

    Element row = hbox({FileBadge(f), text(" " + path_str)});
    if (i == list_selected_) row = row | inverted;
    rows.push_back(row);
  }
  return vbox(std::move(rows)) | focusPosition(0, list_selected_) | frame | flex;
}

Element SourceControlView::RenderTree() {
  Elements rows;
  for (int i = 0; i < static_cast<int>(tree_visible_.size()); ++i) {
    const auto& row = tree_visible_[i];
    std::string indent(static_cast<size_t>(row.depth) * 2, ' ');
    std::string_view glyph = row.node->is_directory
                                  ? Icons::FolderGlyph(row.node->expanded)
                                  : Icons::FileGlyph(row.node->path.extension().string());
    Element line = text(indent + std::string(glyph) + " " + row.node->name);

    if (!row.node->is_directory) {
      line = hbox({line, filler(), FileBadge(row.node->status), text(" ")});
    }

    if (i == tree_selected_) line = line | inverted;
    rows.push_back(line);
  }
  return vbox(std::move(rows)) | focusPosition(0, tree_selected_) | frame | flex;
}

bool SourceControlView::OnEvent(Event event) {
  if (event == Event::F5) {
    if (on_refresh_requested_) on_refresh_requested_();
    return true;
  }
  // Local to this view (like F5 above), not a global CommandRegistry
  // binding -- only meaningful while Source Control is focused.
  if (event == Event::Character("t") || event == Event::Character("T")) {
    mode_ = (mode_ == ScmViewMode::List) ? ScmViewMode::Tree : ScmViewMode::List;
    return true;
  }
  if (!status_.is_repo || status_.files.empty()) return false;

  return mode_ == ScmViewMode::Tree ? OnEventTree(event) : OnEventList(event);
}

bool SourceControlView::OnEventList(Event event) {
  if (event == Event::ArrowUp) {
    list_selected_ = std::max(0, list_selected_ - 1);
    return true;
  }
  if (event == Event::ArrowDown) {
    list_selected_ = std::min(static_cast<int>(status_.files.size()) - 1, list_selected_ + 1);
    return true;
  }
  if (event == Event::Return) {
    if (on_open_) on_open_(status_.files[static_cast<size_t>(list_selected_)].path);
    return true;
  }
  return false;
}

bool SourceControlView::OnEventTree(Event event) {
  if (tree_visible_.empty()) return false;

  if (event == Event::ArrowUp) {
    tree_selected_ = std::max(0, tree_selected_ - 1);
    return true;
  }
  if (event == Event::ArrowDown) {
    tree_selected_ = std::min(static_cast<int>(tree_visible_.size()) - 1, tree_selected_ + 1);
    return true;
  }

  auto* node = tree_visible_[static_cast<size_t>(tree_selected_)].node;
  if (event == Event::Return) {
    if (node->is_directory) {
      tree_.ToggleExpanded(*node);
      RefreshTreeVisible();
    } else if (on_open_) {
      on_open_(node->path);
    }
    return true;
  }
  if (event == Event::ArrowRight) {
    if (node->is_directory && !node->expanded) {
      tree_.ToggleExpanded(*node);
      RefreshTreeVisible();
    }
    return true;
  }
  if (event == Event::ArrowLeft) {
    if (node->is_directory && node->expanded) {
      tree_.ToggleExpanded(*node);
      RefreshTreeVisible();
    }
    return true;
  }
  return false;
}

}  // namespace puka

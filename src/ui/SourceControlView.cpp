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

// One character, not the two-column staged+unstaged porcelain badge used
// elsewhere -- now that a file's staged and unstaged deltas render in their
// own separate sections (see SourceControlView.hpp's staged_/unstaged_
// comment), each row only needs to show the delta relevant to that section.
Element FileBadge(const GitFileStatus& f, bool staged_section) {
  GitDeltaType type = staged_section ? f.staged : f.unstaged;
  return text(std::string(1, BadgeChar(type))) | color(BadgeColor(type));
}

// Shift+Enter isn't a named FTXUI event -- there's no reliable byte
// sequence for it in most terminals without opting into
// modifyOtherKeys/CSI-u (unlike the Alt+<letter> chords elsewhere in this
// app, plain Enter and Shift+Enter are often indistinguishable bytes by
// default). Best-effort support for the two common raw encodings, plus a
// guaranteed-reliable `o` fallback bound to the same action.
bool IsOpenDirectChord(const Event& event) {
  static const Event kShiftReturnLegacy = Event::Special("\x1b[27;2;13~");
  static const Event kShiftReturnCsiU = Event::Special("\x1b[13;2u");
  return event == kShiftReturnLegacy || event == kShiftReturnCsiU ||
         event == Event::Character("o") || event == Event::Character("O");
}

}  // namespace

SourceControlView::SourceControlView(
    std::filesystem::path root,
    std::function<void(const std::filesystem::path&, ScmOpenMode)> on_open,
    std::function<void()> on_refresh_requested)
    : root_(std::move(root)),
      on_open_(std::move(on_open)),
      on_refresh_requested_(std::move(on_refresh_requested)) {}

void SourceControlView::SetStatus(GitRepoStatus status) {
  status_ = std::move(status);

  staged_.clear();
  unstaged_.clear();
  for (const auto& f : status_.files) {
    if (f.staged != GitDeltaType::None) staged_.push_back(f);
    if (f.unstaged != GitDeltaType::None) unstaged_.push_back(f);
  }

  int total = static_cast<int>(staged_.size() + unstaged_.size());
  list_selected_ = total == 0 ? 0 : std::clamp(list_selected_, 0, total - 1);

  tree_staged_.SetFiles(staged_, root_);
  tree_unstaged_.SetFiles(unstaged_, root_);
  RefreshTreeVisible();
}

void SourceControlView::RefreshTreeVisible() {
  tree_visible_ = tree_staged_.VisibleRows();
  tree_staged_row_count_ = tree_visible_.size();
  auto unstaged_rows = tree_unstaged_.VisibleRows();
  tree_visible_.insert(tree_visible_.end(), unstaged_rows.begin(), unstaged_rows.end());

  tree_selected_ = tree_visible_.empty()
                        ? 0
                        : std::clamp(tree_selected_, 0, static_cast<int>(tree_visible_.size()) - 1);
}

const GitFileStatus* SourceControlView::SelectedFileList() const {
  if (list_selected_ < static_cast<int>(staged_.size())) {
    return &staged_[static_cast<size_t>(list_selected_)];
  }
  size_t idx = static_cast<size_t>(list_selected_) - staged_.size();
  return idx < unstaged_.size() ? &unstaged_[idx] : nullptr;
}

ScmTree& SourceControlView::TreeForVisibleIndex(int index) {
  return static_cast<size_t>(index) < tree_staged_row_count_ ? tree_staged_ : tree_unstaged_;
}

Element SourceControlView::OnRender() {
  if (!status_.is_repo) {
    return vbox({filler(), hcenter(text("Not a git repository") | dim), filler()});
  }

  std::string branch_label = (status_.detached ? "(detached) " : "") + status_.branch;
  std::string mode_label = mode_ == ScmViewMode::Tree ? "Tree" : "List";
  Element header = hbox(
      {text(branch_label) | bold | color(Color::Cyan), filler(), text(mode_label) | dim});

  if (staged_.empty() && unstaged_.empty()) {
    return vbox({header, separator(), filler(), hcenter(text("No changes") | dim), filler()});
  }

  Element body = mode_ == ScmViewMode::Tree ? RenderTree() : RenderList();
  return vbox({header, separator(), body});
}

Element SourceControlView::RenderList() {
  Elements rows;
  int visual_selected = 0;
  int file_index = 0;

  // Staged Changes first, then Changes -- same order as VSCode. A section
  // with nothing in it doesn't get a header at all (also matching VSCode),
  // so a repo with only unstaged edits looks exactly like before this
  // split existed.
  auto add_section = [&](const std::vector<GitFileStatus>& files, const char* label,
                          bool staged_section) {
    if (files.empty()) return;
    rows.push_back(text(std::string(label) + " (" + std::to_string(files.size()) + ")") | dim);
    for (const auto& f : files) {
      std::error_code ec;
      auto rel = std::filesystem::relative(f.path, root_, ec);
      std::string path_str = ec ? f.path.string() : rel.string();

      bool selected = (file_index == list_selected_);
      Color name_color = selected ? Color(Color::Black) : Color(Color::Default);
      Element row =
          hbox({FileBadge(f, staged_section), text(" " + path_str) | color(name_color)});
      // Not `| inverted` -- see the comment on the equivalent line in
      // FileTreeView::OnRender: it'd turn the badge's own fg color into a
      // colored background instead of a uniform selection highlight.
      if (selected) {
        row = row | bgcolor(Color::GrayLight);
        visual_selected = static_cast<int>(rows.size());
      }
      rows.push_back(row);
      ++file_index;
    }
  };
  add_section(staged_, "Staged Changes", true);
  add_section(unstaged_, "Changes", false);

  return vbox(std::move(rows)) | focusPosition(0, visual_selected) | frame | flex;
}

Element SourceControlView::RenderTree() {
  Elements rows;
  int visual_selected = 0;

  auto render_row = [&](int i, bool staged_section) {
    const auto& row = tree_visible_[static_cast<size_t>(i)];
    std::string indent(static_cast<size_t>(row.depth) * 2, ' ');
    std::string_view glyph = row.node->is_directory
                                  ? Icons::FolderGlyph(row.node->expanded)
                                  : Icons::FileGlyph(row.node->path.extension().string());
    bool selected = (i == tree_selected_);
    Color glyph_color = row.node->is_directory
                             ? Icons::FolderColor(selected)
                             : Icons::FileColor(row.node->path.extension().string(), selected);
    Color name_color = selected ? Color(Color::Black) : Color(Color::Default);
    Element line = hbox({text(indent), text(std::string(glyph)) | color(glyph_color),
                          text(" " + row.node->name) | color(name_color)});

    if (!row.node->is_directory) {
      line = hbox({line, filler(), FileBadge(row.node->status, staged_section), text(" ")});
    }

    if (selected) {
      line = line | bgcolor(Color::GrayLight);
      visual_selected = static_cast<int>(rows.size());
    }
    rows.push_back(line);
  };

  int i = 0;
  if (tree_staged_row_count_ > 0) rows.push_back(text("Staged Changes") | dim);
  for (; static_cast<size_t>(i) < tree_staged_row_count_; ++i) render_row(i, true);
  if (tree_visible_.size() > tree_staged_row_count_) rows.push_back(text("Changes") | dim);
  for (; i < static_cast<int>(tree_visible_.size()); ++i) render_row(i, false);

  return vbox(std::move(rows)) | focusPosition(0, visual_selected) | frame | flex;
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
  if (!status_.is_repo || (staged_.empty() && unstaged_.empty())) return false;

  return mode_ == ScmViewMode::Tree ? OnEventTree(event) : OnEventList(event);
}

bool SourceControlView::OnEventList(Event event) {
  int total = static_cast<int>(staged_.size() + unstaged_.size());
  if (event == Event::ArrowUp) {
    list_selected_ = std::max(0, list_selected_ - 1);
    return true;
  }
  if (event == Event::ArrowDown) {
    list_selected_ = std::min(total - 1, list_selected_ + 1);
    return true;
  }
  if (event == Event::Return) {
    if (const auto* f = SelectedFileList(); f && on_open_) on_open_(f->path, ScmOpenMode::Diff);
    return true;
  }
  if (IsOpenDirectChord(event)) {
    if (const auto* f = SelectedFileList(); f && on_open_) on_open_(f->path, ScmOpenMode::File);
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
      TreeForVisibleIndex(tree_selected_).ToggleExpanded(*node);
      RefreshTreeVisible();
    } else if (on_open_) {
      on_open_(node->path, ScmOpenMode::Diff);
    }
    return true;
  }
  if (IsOpenDirectChord(event)) {
    if (!node->is_directory && on_open_) on_open_(node->path, ScmOpenMode::File);
    return true;
  }
  if (event == Event::ArrowRight) {
    if (node->is_directory && !node->expanded) {
      TreeForVisibleIndex(tree_selected_).ToggleExpanded(*node);
      RefreshTreeVisible();
    }
    return true;
  }
  if (event == Event::ArrowLeft) {
    if (node->is_directory && node->expanded) {
      TreeForVisibleIndex(tree_selected_).ToggleExpanded(*node);
      RefreshTreeVisible();
    }
    return true;
  }
  return false;
}

}  // namespace puka

#include "ui/StatusBar.hpp"

#include "ui/GitStatusBadge.hpp"
#include "ui/Icons.hpp"

namespace puka {
using namespace ftxui;

namespace {

// One badge per changed-file type with a non-zero count (e.g. "+3 ~2 ?4"),
// same char/color language as the Explorer and Source Control badges --
// collapses each file's staged+unstaged state to one type the same way
// PrimaryDelta already does for Explorer's single-badge-per-row display.
Elements DiffSummaryParts(const GitRepoStatus& git_status) {
  int counts[7] = {};
  for (const auto& f : git_status.files) ++counts[static_cast<int>(PrimaryDelta(f))];

  Elements parts;
  auto add = [&](GitDeltaType type) {
    int n = counts[static_cast<int>(type)];
    if (n == 0) return;
    if (!parts.empty()) parts.push_back(text(" "));
    parts.push_back(text(std::string(1, BadgeChar(type)) + std::to_string(n)) |
                     color(BadgeColor(type)));
  };
  add(GitDeltaType::Added);
  add(GitDeltaType::Modified);
  add(GitDeltaType::Deleted);
  add(GitDeltaType::Renamed);
  add(GitDeltaType::TypeChange);
  add(GitDeltaType::Untracked);
  return parts;
}

}  // namespace

Element RenderStatusBar(const std::string& hint, const GitRepoStatus& git_status) {
  Element left = text("");
  if (git_status.is_repo) {
    std::string repo_name = git_status.repo_root.filename().string();
    std::string branch_label = (git_status.detached ? "(detached) " : "") + git_status.branch;
    std::string label = repo_name.empty() ? branch_label : repo_name + " " + branch_label;

    Element branch_part =
        hbox({text(" "), text(std::string(Icons::SourceControlGlyph())), text(" " + label)}) |
        dim;

    Elements row = {branch_part};
    Elements diff_parts = DiffSummaryParts(git_status);
    if (!diff_parts.empty()) {
      row.push_back(text("  "));
      for (auto& part : diff_parts) row.push_back(std::move(part));
    }
    left = hbox(std::move(row));
  }
  return hbox({left, filler(), text(hint) | dim, text(" ")});
}

}  // namespace puka

#include "ui/SourceControlView.hpp"

#include <algorithm>
#include <system_error>

#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include "ui/GitStatusBadge.hpp"

namespace puka {
using namespace ftxui;

SourceControlView::SourceControlView(std::filesystem::path root,
                                      std::function<void(const std::filesystem::path&)> on_open,
                                      std::function<void()> on_refresh_requested)
    : root_(std::move(root)),
      on_open_(std::move(on_open)),
      on_refresh_requested_(std::move(on_refresh_requested)) {}

void SourceControlView::SetStatus(GitRepoStatus status) {
  status_ = std::move(status);
  selected_ = status_.files.empty()
                  ? 0
                  : std::clamp(selected_, 0, static_cast<int>(status_.files.size()) - 1);
}

Element SourceControlView::OnRender() {
  if (!status_.is_repo) {
    return vbox({filler(), hcenter(text("Not a git repository") | dim), filler()});
  }

  std::string branch_label = (status_.detached ? "(detached) " : "") + status_.branch;
  Element header = text(branch_label) | bold | color(Color::Cyan);

  if (status_.files.empty()) {
    return vbox({header, separator(), filler(), hcenter(text("No changes") | dim), filler()});
  }

  Elements rows;
  for (int i = 0; i < static_cast<int>(status_.files.size()); ++i) {
    const auto& f = status_.files[i];
    // Two-column porcelain-style badge (staged, unstaged) so a file with
    // both a staged change and further unstaged edits on top (e.g. "MM")
    // is represented as one row, not split across separate sections.
    std::string badge = (f.unstaged == GitDeltaType::Untracked)
                             ? "??"
                             : std::string(1, BadgeChar(f.staged)) + BadgeChar(f.unstaged);
    Element badge_el = text(badge) | color(BadgeColor(PrimaryDelta(f)));

    std::error_code ec;
    auto rel = std::filesystem::relative(f.path, root_, ec);
    std::string path_str = ec ? f.path.string() : rel.string();

    Element row = hbox({badge_el, text(" " + path_str)});
    if (i == selected_) row = row | inverted;
    rows.push_back(row);
  }
  Element list = vbox(std::move(rows)) | focusPosition(0, selected_) | frame | flex;
  return vbox({header, separator(), list});
}

bool SourceControlView::OnEvent(Event event) {
  if (event == Event::F5) {
    if (on_refresh_requested_) on_refresh_requested_();
    return true;
  }
  if (!status_.is_repo || status_.files.empty()) return false;

  if (event == Event::ArrowUp) {
    selected_ = std::max(0, selected_ - 1);
    return true;
  }
  if (event == Event::ArrowDown) {
    selected_ = std::min(static_cast<int>(status_.files.size()) - 1, selected_ + 1);
    return true;
  }
  if (event == Event::Return) {
    if (on_open_) on_open_(status_.files[static_cast<size_t>(selected_)].path);
    return true;
  }
  return false;
}

}  // namespace puka

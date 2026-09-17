#pragma once
#include <ftxui/screen/color.hpp>

#include "git/GitService.hpp"

namespace puka {

// Shared by SourceControlView and FileTreeView so a given status always
// renders with the exact same character/color in both places.
char BadgeChar(GitDeltaType type);
ftxui::Color BadgeColor(GitDeltaType type);

// Collapses a file's staged+unstaged state to the single most relevant
// GitDeltaType for Explorer's one-badge-per-row display. Unstaged wins over
// staged -- it's "what you'd see if you looked at this file right now".
GitDeltaType PrimaryDelta(const GitFileStatus& status);

}  // namespace puka

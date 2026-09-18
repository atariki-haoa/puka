#pragma once
#include <string>

#include <ftxui/dom/elements.hpp>

#include "git/GitService.hpp"

namespace puka {

// Non-interactive footer row rendered along the bottom of the window --
// repo/branch on the left (blank when the workspace isn't a git repo),
// right-aligned hint text on the right (e.g. how to open the shortcuts
// popup). Not a Component: it never receives events, so a plain Element
// factory is enough.
ftxui::Element RenderStatusBar(const std::string& hint, const GitRepoStatus& git_status);

}  // namespace puka

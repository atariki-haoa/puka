#include "ui/StatusBar.hpp"

#include "ui/Icons.hpp"

namespace puka {
using namespace ftxui;

Element RenderStatusBar(const std::string& hint, const GitRepoStatus& git_status) {
  Element left = text("");
  if (git_status.is_repo) {
    std::string repo_name = git_status.repo_root.filename().string();
    std::string branch_label = (git_status.detached ? "(detached) " : "") + git_status.branch;
    std::string label = repo_name.empty() ? branch_label : repo_name + " " + branch_label;
    left = hbox({text(" "), text(std::string(Icons::SourceControlGlyph())), text(" " + label)}) |
           dim;
  }
  return hbox({left, filler(), text(hint) | dim, text(" ")});
}

}  // namespace puka

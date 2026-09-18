#pragma once
#include <string>
#include <vector>

#include <ftxui/component/component_base.hpp>

#include "git/GitService.hpp"

namespace puka {

// Read-only, full-pane side-by-side diff of a single file: HEAD content on
// the left, current on-disk content on the right, changed lines colored.
// Opened from Source Control via Enter (Shift+Enter/`o` bypass this and
// open the plain file instead). Meant to be shown via ftxui::Modal(main,
// this, show), which routes input exclusively to this component while
// `*show` is true -- Escape is the only way for control to return to
// `main`, same convention as ShortcutsPopup.
class DiffView : public ftxui::ComponentBase {
 public:
  explicit DiffView(bool* show);

  void SetDiff(std::string title, GitFileDiff diff);

  ftxui::Element OnRender() override;
  bool OnEvent(ftxui::Event event) override;
  // See ShortcutsPopup's identical override for why this is required for a
  // Modal-hosted component to ever receive OnEvent at all.
  bool Focusable() const override { return true; }

 private:
  struct Row {
    bool has_old = false;
    int old_lineno = -1;
    std::string old_text;
    bool has_new = false;
    int new_lineno = -1;
    std::string new_text;
    bool changed = false;  // false => unchanged context row
  };

  void RebuildRows();

  std::string title_;
  GitFileDiff diff_;
  std::vector<Row> rows_;
  int selected_ = 0;
  bool* show_;
};

}  // namespace puka

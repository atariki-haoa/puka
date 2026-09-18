#pragma once
#include <string>
#include <vector>

#include <ftxui/component/component_base.hpp>

namespace puka {

// UI-owned, decoupled from keys::Binding so puka_ui doesn't need to depend on
// puka_keys -- Application translates CommandRegistry::Bindings() into these.
struct ShortcutEntry {
  std::string label;
  std::string description;
};

// Modal popup listing global keyboard shortcuts. Meant to be shown via
// ftxui::Modal(main, popup, show), which routes input exclusively to this
// component while `*show` is true -- closing itself is therefore the only
// way for control to return to `main`.
class ShortcutsPopup : public ftxui::ComponentBase {
 public:
  ShortcutsPopup(std::vector<ShortcutEntry> shortcuts, bool* show);

  ftxui::Element OnRender() override;
  bool OnEvent(ftxui::Event event) override;
  // No children to derive this from (ComponentBase's default Focusable() is
  // "true if any child is") -- without this override, the Container::Tab
  // inside ftxui::Modal never considers itself Focused() while showing this
  // popup, and OnEvent never reaches it, so no key could ever close it.
  bool Focusable() const override { return true; }

 private:
  std::vector<ShortcutEntry> shortcuts_;
  bool* show_;
  int selected_ = 0;
};

}  // namespace puka

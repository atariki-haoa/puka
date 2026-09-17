#include "keys/KeymapDefaults.hpp"

#include <ftxui/component/event.hpp>

namespace puka {
using ftxui::Event;

std::vector<Binding> DefaultKeymap() {
  return {
      // Toggle sidebar. Ctrl+B is VSCode's default. It's also tmux's default
      // prefix key and won't reach the app under tmux; there is currently no
      // fallback chord for this one specifically (Alt+B was reassigned to
      // "focus files" below, at the user's request).
      {Event::CtrlB, "workbench.action.toggleSidebarVisibility"},

      // VSCode uses Ctrl+Shift+E/F/G for these, but terminals collapse
      // Ctrl+<letter> to a single control byte regardless of Shift -- there
      // is no way to distinguish Ctrl+Shift+E from Ctrl+E at this layer, so
      // Alt+<letter> is used instead (reliably disambiguated by FTXUI's
      // ESC-prefix parsing). Alt+B (not Alt+E) is bound to Explorer/files.
      {Event::AltB, "workbench.view.explorer"},
      {Event::AltF, "workbench.view.search"},
      {Event::AltG, "workbench.view.scm"},

      {Event::CtrlS, "workbench.action.files.save"},
      // Some terminals/IDEs (integrated terminals in particular) intercept
      // Ctrl+W themselves to close their own tab/pane -- Alt+W is a fallback
      // that reaches the app either way.
      {Event::CtrlW, "workbench.action.closeActiveEditor"},
      {Event::AltW, "workbench.action.closeActiveEditor"},

      // Escape has no other meaning yet in Phase 1 and is essentially never
      // intercepted by terminals/window managers, so it's the most reliable
      // way to move focus back and forth between the sidebar and the editor
      // (Alt+E/F/G also focuses a specific sidebar view, but Escape doesn't
      // require remembering which one).
      {Event::Escape, "workbench.action.togglePaneFocus"},

      // FTXUI has no named Ctrl+PageUp/PageDown or Alt+Arrow event (and
      // Ctrl+Tab is too often intercepted by terminal emulators/tmux to rely
      // on), so tab switching uses Ctrl+Left/Ctrl+Right instead.
      {Event::ArrowRightCtrl, "workbench.action.nextEditor"},
      {Event::ArrowLeftCtrl, "workbench.action.previousEditor"},

      {Event::CtrlZ, "undo"},
      {Event::CtrlY, "redo"},
  };
}

}  // namespace puka

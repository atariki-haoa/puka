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
      {Event::CtrlB, "workbench.action.toggleSidebarVisibility", "Ctrl+B", "Toggle sidebar"},

      // VSCode uses Ctrl+Shift+E/F/G for these, but terminals collapse
      // Ctrl+<letter> to a single control byte regardless of Shift -- there
      // is no way to distinguish Ctrl+Shift+E from Ctrl+E at this layer, so
      // Alt+<letter> is used instead (reliably disambiguated by FTXUI's
      // ESC-prefix parsing). Alt+B (not Alt+E) is bound to Explorer/files.
      {Event::AltB, "workbench.view.explorer", "Alt+B", "Focus Explorer"},
      {Event::AltF, "workbench.view.search", "Alt+F", "Focus Search"},
      {Event::AltG, "workbench.view.scm", "Alt+G", "Focus Source Control"},

      {Event::CtrlS, "workbench.action.files.save", "Ctrl+S", "Save file"},
      // Some terminals/IDEs (integrated terminals in particular) intercept
      // Ctrl+W themselves to close their own tab/pane -- Alt+W is a fallback
      // that reaches the app either way.
      {Event::CtrlW, "workbench.action.closeActiveEditor", "Ctrl+W", "Close editor tab"},
      {Event::AltW, "workbench.action.closeActiveEditor", "Alt+W", "Close editor tab"},

      // Escape has no other meaning yet in Phase 1 and is essentially never
      // intercepted by terminals/window managers, so it's the most reliable
      // way to move focus back and forth between the sidebar and the editor
      // (Alt+E/F/G also focuses a specific sidebar view, but Escape doesn't
      // require remembering which one).
      {Event::Escape, "workbench.action.togglePaneFocus", "Esc",
       "Switch focus between sidebar and editor"},

      // FTXUI has no named Ctrl+PageUp/PageDown or Alt+Arrow event (and
      // Ctrl+Tab is too often intercepted by terminal emulators/tmux to rely
      // on), so tab switching uses Ctrl+Left/Ctrl+Right instead.
      {Event::ArrowRightCtrl, "workbench.action.nextEditor", "Ctrl+Right", "Next tab"},
      {Event::ArrowLeftCtrl, "workbench.action.previousEditor", "Ctrl+Left", "Previous tab"},

      // Cycle the sidebar's Explorer/Search/Source-Control views. Ctrl+Right
      // Ctrl+Left above already mean "switch editor tab", so this uses
      // Alt+Right/Alt+Left instead -- FTXUI has no named event for
      // Alt+Arrow, so these are the raw xterm CSI sequences (`ESC[1;3C` /
      // `ESC[1;3D`), the same trick FTXUI itself uses internally to define
      // ArrowRightCtrl/ArrowLeftCtrl above. Reaches puka in most modern
      // terminal emulators, but unlike the Alt+<letter> bindings (which
      // FTXUI parses reliably via its own ESC-prefix logic) isn't
      // guaranteed universally -- Alt+B/F/G above remain the reliable way
      // to jump straight to a given view.
      {Event::Special("\x1B[1;3C"), "workbench.action.nextSidebarView", "Alt+Right",
       "Next sidebar view"},
      {Event::Special("\x1B[1;3D"), "workbench.action.previousSidebarView", "Alt+Left",
       "Previous sidebar view"},

      {Event::CtrlZ, "undo", "Ctrl+Z", "Undo"},
      {Event::CtrlY, "redo", "Ctrl+Y", "Redo"},

      // F1 is the conventional "help" key and, unlike punctuation like '?',
      // is never a character a user would type into search/editor text, so
      // it's safe to bind globally without shadowing normal input.
      {Event::F1, "workbench.action.toggleShortcutsHelp", "F1", "to info"},
  };
}

}  // namespace puka

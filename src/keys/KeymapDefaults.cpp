#include "keys/KeymapDefaults.hpp"

#include <ftxui/component/event.hpp>

namespace puka {
using ftxui::Event;

namespace {

// Alt+1..Alt+9 jump-to-tab bindings (VSCode's own chord is Ctrl+1..Ctrl+9,
// but that's not usable here: classic xterm encoding has no distinct C0 code
// for most Ctrl+<digit> combinations, and the ones that do exist collide with
// keys already bound elsewhere -- Ctrl+3 sends the same single ESC byte
// (0x1B) as the Escape key, and Ctrl+8 commonly sends the same DEL byte as
// Backspace. Alt+<digit> is encoded the exact same ESC-prefix way as the
// Alt+<letter> bindings above (see AltB/AltF/AltG's own comment), so it's
// exactly as reliable and doesn't collide with anything.
std::vector<Binding> TabIndexBindings() {
  std::vector<Binding> bindings;
  for (int i = 1; i <= 9; ++i) {
    std::string digit(1, static_cast<char>('0' + i));
    bindings.push_back({Event::Special(std::string("\x1B") + digit),
                         "workbench.action.openEditorAtIndex" + digit, "Alt+" + digit,
                         "Go to tab " + digit});
  }
  return bindings;
}

}  // namespace

std::vector<Binding> DefaultKeymap() {
  std::vector<Binding> bindings = {
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
      // Ctrl+F sends a single raw control byte (\x06), same as Ctrl+S/Ctrl+Z
      // above -- unlike the Alt+<letter> chords, there's no terminal-specific
      // interception risk to design around here.
      {Event::CtrlF, "editor.action.find", "Ctrl+F", "Find in file"},
      // Ctrl+K (\x0b) -- readline/emacs binds this at the shell's own
      // line-editing layer, which doesn't apply once puka has the terminal
      // in raw mode, so it reaches the app like any other Ctrl+<letter>
      // chord here.
      {Event::CtrlK, "editor.action.deleteLine", "Ctrl+K", "Delete line"},
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

      // Jumping directly to a tab by number: see TabIndexBindings()'s own
      // comment above for why this is Alt+<digit>, not VSCode's Ctrl+<digit>.

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

      // Ctrl+C already exits (FTXUI raises SIGINT for any unhandled
      // Ctrl+C), but that's a signal-driven kill, not a real "quit"
      // command. Ctrl+Q is the common IDE/terminal-app quit chord and, like
      // Ctrl+S above, reaches the app fine despite historically being an
      // XON/XOFF flow-control byte -- FTXUI's raw terminal mode disables
      // that.
      {Event::CtrlQ, "workbench.action.quit", "Ctrl+Q", "Quit puka"},
  };

  auto tab_index_bindings = TabIndexBindings();
  bindings.insert(bindings.end(), tab_index_bindings.begin(), tab_index_bindings.end());
  return bindings;
}

}  // namespace puka

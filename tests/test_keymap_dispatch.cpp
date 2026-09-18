#include <map>
#include <string>

#include <ftxui/component/event.hpp>

#include "keys/CommandRegistry.hpp"
#include "keys/KeymapDefaults.hpp"
#include "test_util.hpp"

using namespace puka;
using namespace puka::test;
using ftxui::Event;

namespace {

void TestNoChordBoundToMultipleDistinctCommands() {
  auto bindings = DefaultKeymap();
  std::map<std::string, std::string> chord_to_command;  // keyed by Event::input()
  for (const auto& binding : bindings) {
    auto [it, inserted] = chord_to_command.emplace(binding.chord.input(), binding.command);
    if (!inserted) {
      Check(it->second == binding.command,
            "chord bound to two different commands: " + it->second + " vs " + binding.command);
    }
  }
}

void TestDispatchInvokesRegisteredHandler() {
  CommandRegistry registry(DefaultKeymap());
  bool called = false;
  registry.Register("workbench.action.files.save", [&called] { called = true; });
  bool handled = registry.Dispatch("workbench.action.files.save");
  Check(handled, "dispatch reports the command as handled");
  Check(called, "dispatch actually invoked the registered handler");
}

void TestDispatchUnregisteredCommandIsSafeNoop() {
  CommandRegistry registry(DefaultKeymap());
  bool handled = registry.Dispatch("some.unregistered.command");
  Check(!handled, "dispatching an unregistered command returns false");
}

void TestCommandForChordFindsSecondaryBinding() {
  CommandRegistry registry(DefaultKeymap());
  auto ctrl_w = registry.CommandForChord(Event::CtrlW);
  auto alt_w = registry.CommandForChord(Event::AltW);
  Check(ctrl_w.has_value() && *ctrl_w == "workbench.action.closeActiveEditor",
        "Ctrl+W resolves to close tab");
  Check(alt_w.has_value() && *alt_w == "workbench.action.closeActiveEditor",
        "Alt+W resolves to the same command as its terminal-safe fallback");
}

void TestAltBFocusesExplorerNotToggleSidebar() {
  CommandRegistry registry(DefaultKeymap());
  auto ctrl_b = registry.CommandForChord(Event::CtrlB);
  auto alt_b = registry.CommandForChord(Event::AltB);
  Check(ctrl_b.has_value() && *ctrl_b == "workbench.action.toggleSidebarVisibility",
        "Ctrl+B resolves to toggle sidebar");
  Check(alt_b.has_value() && *alt_b == "workbench.view.explorer",
        "Alt+B resolves to focus Explorer/files, not toggle sidebar");
}

void TestF1ShowsShortcutsHelp() {
  CommandRegistry registry(DefaultKeymap());
  auto f1 = registry.CommandForChord(Event::F1);
  Check(f1.has_value() && *f1 == "workbench.action.toggleShortcutsHelp",
        "F1 resolves to toggling the shortcuts popup");
}

void TestAltArrowsCycleSidebarView() {
  CommandRegistry registry(DefaultKeymap());
  auto next = registry.CommandForChord(Event::Special("\x1B[1;3C"));
  auto prev = registry.CommandForChord(Event::Special("\x1B[1;3D"));
  Check(next.has_value() && *next == "workbench.action.nextSidebarView",
        "Alt+Right resolves to cycling to the next sidebar view");
  Check(prev.has_value() && *prev == "workbench.action.previousSidebarView",
        "Alt+Left resolves to cycling to the previous sidebar view");
}

}  // namespace

int main() {
  TestNoChordBoundToMultipleDistinctCommands();
  TestDispatchInvokesRegisteredHandler();
  TestDispatchUnregisteredCommandIsSafeNoop();
  TestCommandForChordFindsSecondaryBinding();
  TestAltBFocusesExplorerNotToggleSidebar();
  TestF1ShowsShortcutsHelp();
  TestAltArrowsCycleSidebarView();

  if (g_failures == 0) {
    std::cout << "test_keymap_dispatch: all tests passed\n";
    return 0;
  }
  std::cerr << "test_keymap_dispatch: " << g_failures << " failure(s)\n";
  return 1;
}

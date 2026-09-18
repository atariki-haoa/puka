#pragma once
#include <string>

#include <ftxui/component/event.hpp>

namespace puka {

struct Binding {
  ftxui::Event chord;
  std::string command;
  // Human-readable chord and action, e.g. "Ctrl+B" / "Toggle sidebar" -- used
  // to render the keyboard shortcuts popup. Kept on the same table as the
  // chord->command mapping so the popup can't drift out of sync with it.
  std::string label;
  std::string description;
};

}  // namespace puka

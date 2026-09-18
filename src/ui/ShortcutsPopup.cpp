#include "ui/ShortcutsPopup.hpp"

#include <algorithm>

#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/terminal.hpp>

namespace puka {
using namespace ftxui;

ShortcutsPopup::ShortcutsPopup(std::vector<ShortcutEntry> shortcuts, bool* show)
    : shortcuts_(std::move(shortcuts)), show_(show) {}

Element ShortcutsPopup::OnRender() {
  size_t label_width = 0;
  for (const auto& s : shortcuts_) label_width = std::max(label_width, s.label.size());

  Elements rows;
  for (const auto& s : shortcuts_) {
    std::string padded_label = s.label + std::string(label_width - s.label.size(), ' ');
    rows.push_back(hbox({text(padded_label) | bold | color(Color::Cyan), text("  "),
                          text(s.description)}));
  }

  // The list keeps growing as shortcuts get added, so it's kept inside a
  // height-bounded, scrollable frame (Up/Down move `selected_`, which
  // `focusPosition` uses to scroll it into view) rather than letting a tall
  // list overflow past a short terminal with no way to see the rest.
  int max_visible = std::max(5, Terminal::Size().dimy - 8);
  Element list =
      vbox(std::move(rows)) | focusPosition(0, selected_) | frame | size(HEIGHT, LESS_THAN, max_visible);

  return vbox({
             hcenter(text("Keyboard Shortcuts") | bold),
             separator(),
             list,
             separator(),
             hcenter(text("Esc / Enter / F1 to close, Up/Down to scroll") | dim),
         }) |
         border;
}

bool ShortcutsPopup::OnEvent(Event event) {
  // This component only ever receives events while it's the active modal
  // (see the class comment), so any key is fine to close on -- but keeping
  // it to the three "this means close" keys avoids a stray keystroke (e.g.
  // while scrolling in another view muscle-memory) silently dismissing it.
  if (event == Event::Escape || event == Event::Return || event == Event::F1) {
    *show_ = false;
    return true;
  }
  if (event == Event::ArrowUp) {
    selected_ = std::max(0, selected_ - 1);
  } else if (event == Event::ArrowDown) {
    selected_ = std::min(static_cast<int>(shortcuts_.size()) - 1, selected_ + 1);
  }
  return true;
}

}  // namespace puka

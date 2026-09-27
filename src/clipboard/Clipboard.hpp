#pragma once
#include <string>
#include <string_view>

namespace puka {

// Best-effort OS clipboard write for Ctrl+C/Ctrl+X -- puka itself never
// reads the clipboard back: pasting already works today via the terminal's
// own native paste, which delivers pasted bytes as ordinary character
// events that EditorView/Document already handle like typed input.
class Clipboard {
 public:
  // Tries a platform-native external tool first (pbcopy on macOS; wl-copy,
  // xclip, then xsel on Linux, depending on session type and what's on
  // PATH), then falls back to emitting an OSC 52 terminal escape sequence
  // (works even over SSH in terminals that support it -- kitty, iTerm2,
  // alacritty, wezterm, etc). There is no reliable way to confirm OSC 52
  // actually reached the system clipboard (no ack channel), so this always
  // returns true once the fallback was attempted -- the return value only
  // distinguishes "nothing was even tried" (impossible in practice) from
  // "some path was attempted".
  static bool Write(std::string_view text);
};

// Exposed for unit testing -- pure, no I/O. Base64-encodes `text` into an
// OSC 52 "set clipboard" sequence (`\x1b]52;c;<base64>\x07`), wrapped in
// tmux's DCS passthrough envelope when `inside_tmux` is true so a `set -g
// set-clipboard on` tmux config still forwards it to the outer terminal.
std::string BuildOsc52Sequence(std::string_view text, bool inside_tmux);

}  // namespace puka

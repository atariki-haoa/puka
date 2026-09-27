#include "clipboard/Clipboard.hpp"

#include <cstdio>
#include <cstdlib>
#include <iostream>

namespace puka {

namespace {

const char kBase64Chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string Base64Encode(std::string_view data) {
  std::string out;
  out.reserve((data.size() + 2) / 3 * 4);
  size_t i = 0;
  for (; i + 3 <= data.size(); i += 3) {
    auto b0 = static_cast<unsigned char>(data[i]);
    auto b1 = static_cast<unsigned char>(data[i + 1]);
    auto b2 = static_cast<unsigned char>(data[i + 2]);
    out += kBase64Chars[b0 >> 2];
    out += kBase64Chars[((b0 & 0x03) << 4) | (b1 >> 4)];
    out += kBase64Chars[((b1 & 0x0F) << 2) | (b2 >> 6)];
    out += kBase64Chars[b2 & 0x3F];
  }
  size_t remaining = data.size() - i;
  if (remaining == 1) {
    auto b0 = static_cast<unsigned char>(data[i]);
    out += kBase64Chars[b0 >> 2];
    out += kBase64Chars[(b0 & 0x03) << 4];
    out += "==";
  } else if (remaining == 2) {
    auto b0 = static_cast<unsigned char>(data[i]);
    auto b1 = static_cast<unsigned char>(data[i + 1]);
    out += kBase64Chars[b0 >> 2];
    out += kBase64Chars[((b0 & 0x03) << 4) | (b1 >> 4)];
    out += kBase64Chars[(b1 & 0x0F) << 2];
    out += "=";
  }
  return out;
}

// Runs `command` (always a fixed literal, never built from `text` -- no
// shell-injection concern) via a shell, writing `text` to its stdin.
// popen() itself succeeds even for a command that doesn't exist (the shell
// it spawns just fails to exec and exits non-zero), so it's pclose()'s exit
// status, not the popen() call, that actually tells "not installed" apart
// from "ran and wrote to the clipboard".
bool TryExternalTool(const char* command, std::string_view text) {
  FILE* pipe = popen(command, "w");
  if (!pipe) return false;
  if (!text.empty()) fwrite(text.data(), 1, text.size(), pipe);
  return pclose(pipe) == 0;
}

}  // namespace

std::string BuildOsc52Sequence(std::string_view text, bool inside_tmux) {
  std::string osc52 = "\x1b]52;c;" + Base64Encode(text) + "\x07";
  if (!inside_tmux) return osc52;

  // tmux only forwards escape sequences it recognizes to the outer
  // terminal; anything else -- including OSC 52 -- has to be wrapped in its
  // DCS passthrough envelope, with every literal ESC byte inside doubled,
  // for `set -g set-clipboard on` setups to actually receive it.
  std::string wrapped = "\x1bPtmux;";
  for (char c : osc52) {
    if (c == '\x1b') wrapped += '\x1b';
    wrapped += c;
  }
  wrapped += "\x1b\\";
  return wrapped;
}

bool Clipboard::Write(std::string_view text) {
#ifdef __APPLE__
  if (TryExternalTool("pbcopy", text)) return true;
#else
  if (getenv("WAYLAND_DISPLAY") != nullptr && TryExternalTool("wl-copy", text)) return true;
  if (TryExternalTool("xclip -selection clipboard -in", text)) return true;
  if (TryExternalTool("xsel --clipboard --input", text)) return true;
#endif

  // Every external tool failed, or this isn't a graphical session at all
  // (e.g. a bare SSH connection) -- fall back to OSC 52. Safe to interleave
  // with FTXUI's own std::cout-based frame rendering (see App::Draw() in
  // vendored ftxui, which also just writes to std::cout): both only ever run
  // on this single event-loop thread, and OSC 52 has no visible glyphs or
  // cursor movement for a subsequent full-frame redraw to corrupt.
  std::cout << BuildOsc52Sequence(text, getenv("TMUX") != nullptr) << std::flush;
  return true;
}

}  // namespace puka

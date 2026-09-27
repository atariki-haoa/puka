#include "clipboard/Clipboard.hpp"
#include "test_util.hpp"

using namespace puka;
using namespace puka::test;

namespace {

void TestPlainSequenceHasNoTmuxWrapping() {
  // "hi" base64-encodes to "aGk=" -- verified against a known-good encoder.
  std::string seq = BuildOsc52Sequence("hi", /*inside_tmux=*/false);
  Check(seq == "\x1b]52;c;aGk=\x07", "plain OSC 52 sequence for a known short input");
}

void TestTmuxWrapsInDcsPassthroughWithDoubledEscapes() {
  std::string plain = BuildOsc52Sequence("hi", /*inside_tmux=*/false);
  std::string wrapped = BuildOsc52Sequence("hi", /*inside_tmux=*/true);

  Check(wrapped.rfind("\x1bPtmux;", 0) == 0, "tmux-wrapped sequence starts with the DCS passthrough prefix");
  Check(wrapped.substr(wrapped.size() - 2) == "\x1b\\", "tmux-wrapped sequence ends with the ST terminator");

  // Every literal ESC byte inside the plain sequence must be doubled once
  // it's embedded in the tmux passthrough envelope.
  size_t escapes_in_plain = 0;
  for (char c : plain) {
    if (c == '\x1b') ++escapes_in_plain;
  }
  size_t escapes_in_wrapped = 0;
  for (char c : wrapped) {
    if (c == '\x1b') ++escapes_in_wrapped;
  }
  // +2 for the envelope's own opening ESC (in "\x1bPtmux;") and closing ESC
  // (in the "\x1b\\" terminator), on top of each inner ESC being doubled.
  Check(escapes_in_wrapped == escapes_in_plain * 2 + 2,
        "every inner ESC is doubled, plus the envelope's own two ESCs");
}

void TestEmptyTextEncodesToEmptyPayload() {
  std::string seq = BuildOsc52Sequence("", /*inside_tmux=*/false);
  Check(seq == "\x1b]52;c;\x07", "empty text encodes to an empty base64 payload");
}

}  // namespace

int main() {
  TestPlainSequenceHasNoTmuxWrapping();
  TestTmuxWrapsInDcsPassthroughWithDoubledEscapes();
  TestEmptyTextEncodesToEmptyPayload();

  if (g_failures == 0) {
    std::cout << "test_clipboard_osc52: all tests passed\n";
    return 0;
  }
  std::cerr << "test_clipboard_osc52: " << g_failures << " failure(s)\n";
  return 1;
}

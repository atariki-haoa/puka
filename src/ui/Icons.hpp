#pragma once
#include <string_view>

namespace puka {

// Nerd Font glyphs are on by default. There is no reliable runtime way to
// detect whether the terminal's active font actually contains them -- a
// missing glyph renders as a broken tofu/box character -- so `--no-nerd-font`
// is available to fall back to the plain-text glyphs.
class Icons {
 public:
  static void SetUseNerdFont(bool enabled);
  static bool UseNerdFont();

  static std::string_view FileGlyph(std::string_view extension);
  static std::string_view FolderGlyph(bool expanded);

  static std::string_view ExplorerGlyph();
  static std::string_view SearchGlyph();
  static std::string_view SourceControlGlyph();

 private:
  static bool use_nerd_font_;
};

}  // namespace puka

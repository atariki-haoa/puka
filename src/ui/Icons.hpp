#pragma once
#include <string_view>

namespace puka {

// Nerd Font glyphs are opt-in (default OFF): there is no reliable runtime way
// to detect whether the terminal's active font actually contains them, and a
// missing glyph renders as a broken tofu/box character, which looks worse
// than the plain-text fallback that's used by default.
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

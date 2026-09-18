#pragma once
#include <string_view>

#include <ftxui/screen/color.hpp>

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

  // Per-extension accent color for the file tree glyph (approximating the
  // common devicon/Seti-UI associations, e.g. Rust orange, Go cyan). Unknown
  // extensions fall back to Color::Default so generic files stay unstyled.
  // `selected` darkens the result -- several of these devicon colors (pale
  // yellow, light blue-grey) were picked against a dark background and
  // nearly vanish on the light-grey cursor row; pass true while rendering a
  // selected row to keep the icon legible there.
  static ftxui::Color FileColor(std::string_view extension, bool selected = false);
  static ftxui::Color FolderColor(bool selected = false);

  static std::string_view ExplorerGlyph();
  static std::string_view SearchGlyph();
  static std::string_view SourceControlGlyph();

 private:
  static bool use_nerd_font_;
};

}  // namespace puka

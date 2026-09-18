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

  // One accent hue per sidebar panel, kept apart from the file-type/badge
  // palette so the activity rail reads as its own thing at a glance.
  // `selected=false` (the default) gives the accent itself, used as the
  // glyph's foreground on the ordinary background for an inactive tab.
  // `selected=true` gives the foreground to pair with that *same* accent
  // used as a bgcolor() fill on the active tab -- a contrast switch, not a
  // darken, since the active tab's whole cell becomes the accent color
  // rather than staying on the default background (contrast FileColor(),
  // which darkens the same hue for a selected row's light-grey background).
  static ftxui::Color ExplorerColor(bool selected = false);
  static ftxui::Color SearchColor(bool selected = false);
  static ftxui::Color SourceControlColor(bool selected = false);

 private:
  static bool use_nerd_font_;
};

}  // namespace puka

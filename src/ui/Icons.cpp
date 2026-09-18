#include "ui/Icons.hpp"

#include <cstdint>
#include <unordered_map>

namespace puka {

bool Icons::use_nerd_font_ = true;

void Icons::SetUseNerdFont(bool enabled) { use_nerd_font_ = enabled; }
bool Icons::UseNerdFont() { return use_nerd_font_; }

namespace {

struct Glyphs {
  std::string_view nerd_font;
  std::string_view fallback;
};

// Nerd Font codepoints below are the commonly-used Seti-UI/Devicons values.
// This path is opt-in and off by default (see Icons.hpp) -- verify visually
// against your own patched font if you enable --nerd-font.
const std::unordered_map<std::string_view, Glyphs>& ExtensionTable() {
  static const std::unordered_map<std::string_view, Glyphs> table = {
      {".cpp", {"", "c+"}},   {".cc", {"", "c+"}},
      {".hpp", {"", "h+"}},   {".h", {"", "h"}},
      {".c", {"", "c"}},      {".py", {"", "py"}},
      {".js", {"", "js"}},    {".jsx", {"", "jx"}},
      {".ts", {"", "ts"}},    {".tsx", {"", "tx"}},
      {".json", {"", "{}"}},  {".md", {"", "md"}},
      {".sh", {"", "sh"}},    {".rs", {"", "rs"}},
      {".go", {"", "go"}},    {".yml", {"", "yl"}},
      {".yaml", {"", "yl"}},  {".toml", {"", "tm"}},
      {".txt", {"", "tx"}},
  };
  return table;
}

}  // namespace

std::string_view Icons::FileGlyph(std::string_view extension) {
  static constexpr Glyphs kGeneric{"", "f"};
  const auto& table = ExtensionTable();
  auto it = table.find(extension);
  const Glyphs& glyphs = (it != table.end()) ? it->second : kGeneric;
  return use_nerd_font_ ? glyphs.nerd_font : glyphs.fallback;
}

std::string_view Icons::FolderGlyph(bool expanded) {
  static constexpr Glyphs kClosed{"", ">"};
  static constexpr Glyphs kOpen{"", "v"};
  const Glyphs& glyphs = expanded ? kOpen : kClosed;
  return use_nerd_font_ ? glyphs.nerd_font : glyphs.fallback;
}

namespace {

struct Rgb {
  uint8_t r, g, b;
};

// Approximate devicon/Seti-UI colors so file kinds stay visually
// distinguishable at a glance (Rust orange, Go cyan, JSON gold, etc).
const std::unordered_map<std::string_view, Rgb>& ExtensionColorTable() {
  static const std::unordered_map<std::string_view, Rgb> table = {
      {".cpp", {0x51, 0x9A, 0xBA}}, {".cc", {0x51, 0x9A, 0xBA}},
      {".hpp", {0x51, 0x9A, 0xBA}}, {".h", {0xA8, 0xB9, 0xCC}},
      {".c", {0xA8, 0xB9, 0xCC}},   {".py", {0xFF, 0xD4, 0x3B}},
      {".js", {0xF1, 0xE0, 0x5A}},  {".jsx", {0x61, 0xDA, 0xFB}},
      {".ts", {0x31, 0x78, 0xC6}},  {".tsx", {0x31, 0x78, 0xC6}},
      {".json", {0xCB, 0xCB, 0x41}}, {".md", {0xB0, 0xB0, 0xB0}},
      {".sh", {0x4E, 0xAA, 0x25}},  {".rs", {0xDE, 0xA5, 0x84}},
      {".go", {0x00, 0xAD, 0xD8}},  {".yml", {0xA0, 0x52, 0xA5}},
      {".yaml", {0xA0, 0x52, 0xA5}}, {".toml", {0x9C, 0x4A, 0x1A}},
      {".txt", {0xCC, 0xCC, 0xCC}},
  };
  return table;
}

constexpr Rgb kFolderRgb{0x42, 0xA5, 0xF5};

// Activity-rail accents: Explorer reuses the same blue as the folder icon
// (both are "files" in spirit); Search is magnifier-glass amber; Source
// Control is Git's own brand orange -- three hues spread far enough apart
// (blue / yellow-amber / red-orange) to stay distinguishable even on
// low-color terminals, and none of them collides with GitStatusBadge's
// green/tan/red/teal/purple delta colors used elsewhere in the same UI.
constexpr Rgb kExplorerAccent{0x42, 0xA5, 0xF5};
constexpr Rgb kSearchAccent{0xFF, 0xCA, 0x28};
constexpr Rgb kSourceControlAccent{0xF0, 0x50, 0x33};

// ~45% brightness -- enough to pull even the palest entries (Python's pale
// yellow, C's pale blue-grey) down to something that still reads clearly
// against the light-grey cursor row, without going so dark the hue stops
// being recognizable.
uint8_t Darken(uint8_t channel) { return static_cast<uint8_t>(channel * 0.45f); }

ftxui::Color ToColor(Rgb rgb, bool selected) {
  if (selected) return ftxui::Color::RGB(Darken(rgb.r), Darken(rgb.g), Darken(rgb.b));
  return ftxui::Color::RGB(rgb.r, rgb.g, rgb.b);
}

}  // namespace

ftxui::Color Icons::FileColor(std::string_view extension, bool selected) {
  const auto& table = ExtensionColorTable();
  auto it = table.find(extension);
  if (it == table.end()) {
    return selected ? ftxui::Color(ftxui::Color::Black) : ftxui::Color(ftxui::Color::Default);
  }
  return ToColor(it->second, selected);
}

ftxui::Color Icons::FolderColor(bool selected) { return ToColor(kFolderRgb, selected); }

std::string_view Icons::ExplorerGlyph() {
  static constexpr Glyphs g{"", "fi"};
  return use_nerd_font_ ? g.nerd_font : g.fallback;
}

std::string_view Icons::SearchGlyph() {
  static constexpr Glyphs g{"", "se"};
  return use_nerd_font_ ? g.nerd_font : g.fallback;
}

std::string_view Icons::SourceControlGlyph() {
  static constexpr Glyphs g{"", "gt"};
  return use_nerd_font_ ? g.nerd_font : g.fallback;
}

// Black reads cleanly on both the blue and amber fills (mid-to-high
// brightness); the orange fill is darker, so white is the one that stays
// legible there instead.
ftxui::Color Icons::ExplorerColor(bool selected) {
  return selected ? ftxui::Color::Black : ToColor(kExplorerAccent, false);
}

ftxui::Color Icons::SearchColor(bool selected) {
  return selected ? ftxui::Color::Black : ToColor(kSearchAccent, false);
}

ftxui::Color Icons::SourceControlColor(bool selected) {
  return selected ? ftxui::Color::White : ToColor(kSourceControlAccent, false);
}

}  // namespace puka

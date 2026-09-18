#include "ui/Icons.hpp"

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

// Approximate devicon/Seti-UI colors so file kinds stay visually
// distinguishable at a glance (Rust orange, Go cyan, JSON gold, etc).
const std::unordered_map<std::string_view, ftxui::Color>& ExtensionColorTable() {
  using ftxui::Color;
  static const std::unordered_map<std::string_view, Color> table = {
      {".cpp", Color::RGB(0x51, 0x9A, 0xBA)}, {".cc", Color::RGB(0x51, 0x9A, 0xBA)},
      {".hpp", Color::RGB(0x51, 0x9A, 0xBA)}, {".h", Color::RGB(0xA8, 0xB9, 0xCC)},
      {".c", Color::RGB(0xA8, 0xB9, 0xCC)},   {".py", Color::RGB(0xFF, 0xD4, 0x3B)},
      {".js", Color::RGB(0xF1, 0xE0, 0x5A)},  {".jsx", Color::RGB(0x61, 0xDA, 0xFB)},
      {".ts", Color::RGB(0x31, 0x78, 0xC6)},  {".tsx", Color::RGB(0x31, 0x78, 0xC6)},
      {".json", Color::RGB(0xCB, 0xCB, 0x41)}, {".md", Color::RGB(0xB0, 0xB0, 0xB0)},
      {".sh", Color::RGB(0x4E, 0xAA, 0x25)},  {".rs", Color::RGB(0xDE, 0xA5, 0x84)},
      {".go", Color::RGB(0x00, 0xAD, 0xD8)},  {".yml", Color::RGB(0xA0, 0x52, 0xA5)},
      {".yaml", Color::RGB(0xA0, 0x52, 0xA5)}, {".toml", Color::RGB(0x9C, 0x4A, 0x1A)},
      {".txt", Color::RGB(0xCC, 0xCC, 0xCC)},
  };
  return table;
}

}  // namespace

ftxui::Color Icons::FileColor(std::string_view extension) {
  const auto& table = ExtensionColorTable();
  auto it = table.find(extension);
  return it != table.end() ? it->second : ftxui::Color::Default;
}

ftxui::Color Icons::FolderColor() { return ftxui::Color::RGB(0x42, 0xA5, 0xF5); }

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

}  // namespace puka

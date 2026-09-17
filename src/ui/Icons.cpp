#include "ui/Icons.hpp"

#include <unordered_map>

namespace puka {

bool Icons::use_nerd_font_ = false;

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

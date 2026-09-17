#include "syntax/Theme.hpp"

#include <unordered_map>

namespace puka {
using namespace ftxui;

namespace {

struct Style {
  Color color;
  bool italic = false;
};

const std::unordered_map<std::string_view, Style>& StyleTable() {
  static const std::unordered_map<std::string_view, Style> table = {
      {"keyword", {Color::RGB(0x56, 0x9C, 0xD6)}},
      {"string", {Color::RGB(0xCE, 0x91, 0x78)}},
      {"string.special", {Color::RGB(0xD7, 0xBA, 0x7D)}},
      {"escape", {Color::RGB(0xD7, 0xBA, 0x7D)}},
      {"string.special.key", {Color::RGB(0x9C, 0xDC, 0xFE)}},
      {"comment", {Color::RGB(0x6A, 0x99, 0x55), true}},
      {"function", {Color::RGB(0xDC, 0xDC, 0xAA)}},
      {"function.method", {Color::RGB(0xDC, 0xDC, 0xAA)}},
      {"function.builtin", {Color::RGB(0xDC, 0xDC, 0xAA)}},
      {"function.special", {Color::RGB(0xDC, 0xDC, 0xAA)}},
      {"type", {Color::RGB(0x4E, 0xC9, 0xB0)}},
      {"type.builtin", {Color::RGB(0x4E, 0xC9, 0xB0)}},
      {"constructor", {Color::RGB(0x4E, 0xC9, 0xB0)}},
      {"constant", {Color::RGB(0x4F, 0xC1, 0xFF)}},
      {"constant.builtin", {Color::RGB(0x56, 0x9C, 0xD6)}},
      {"variable.builtin", {Color::RGB(0x56, 0x9C, 0xD6)}},
      {"variable.parameter", {Color::RGB(0x9C, 0xDC, 0xFE)}},
      {"property", {Color::RGB(0x9C, 0xDC, 0xFE)}},
      {"number", {Color::RGB(0xB5, 0xCE, 0xA8)}},
  };
  return table;
}

}  // namespace

Element ApplyCaptureStyle(std::string_view capture_name, Element element) {
  const auto& table = StyleTable();
  std::string_view name = capture_name;
  while (!name.empty()) {
    auto it = table.find(name);
    if (it != table.end()) {
      element = element | color(it->second.color);
      if (it->second.italic) element = element | italic;
      return element;
    }
    size_t dot = name.rfind('.');
    if (dot == std::string_view::npos) break;
    name = name.substr(0, dot);
  }
  return element;
}

}  // namespace puka

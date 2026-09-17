#include "ui/GitStatusBadge.hpp"

namespace puka {
using namespace ftxui;

char BadgeChar(GitDeltaType type) {
  switch (type) {
    case GitDeltaType::None: return ' ';
    case GitDeltaType::Added: return 'A';
    case GitDeltaType::Modified: return 'M';
    case GitDeltaType::Deleted: return 'D';
    case GitDeltaType::Renamed: return 'R';
    case GitDeltaType::TypeChange: return 'T';
    case GitDeltaType::Untracked: return '?';
  }
  return ' ';
}

Color BadgeColor(GitDeltaType type) {
  switch (type) {
    case GitDeltaType::None: return Color::Default;
    case GitDeltaType::Added: return Color::RGB(0x89, 0xD1, 0x85);
    case GitDeltaType::Modified: return Color::RGB(0xE2, 0xC0, 0x8D);
    case GitDeltaType::Deleted: return Color::RGB(0xF1, 0x4C, 0x4C);
    case GitDeltaType::Renamed: return Color::RGB(0x4E, 0xC9, 0xB0);
    case GitDeltaType::TypeChange: return Color::RGB(0xC5, 0x86, 0xC0);
    case GitDeltaType::Untracked: return Color::RGB(0x89, 0xD1, 0x85);
  }
  return Color::Default;
}

GitDeltaType PrimaryDelta(const GitFileStatus& status) {
  return status.unstaged != GitDeltaType::None ? status.unstaged : status.staged;
}

}  // namespace puka

#pragma once
#include <vector>

#include "keys/KeyBinding.hpp"

namespace puka {

// Phase 1 keybinding table. See README.md for why a few of these diverge
// from VSCode's literal defaults (terminal I/O can't deliver every chord).
std::vector<Binding> DefaultKeymap();

}  // namespace puka

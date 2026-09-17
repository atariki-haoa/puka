#pragma once
#include <string>

#include <ftxui/component/event.hpp>

namespace puka {

struct Binding {
  ftxui::Event chord;
  std::string command;
};

}  // namespace puka

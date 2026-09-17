#include "keys/CommandRegistry.hpp"

namespace puka {

CommandRegistry::CommandRegistry(std::vector<Binding> bindings) : bindings_(std::move(bindings)) {}

void CommandRegistry::Register(const std::string& command, std::function<void()> handler) {
  handlers_[command] = std::move(handler);
}

bool CommandRegistry::Dispatch(const std::string& command) const {
  auto it = handlers_.find(command);
  if (it == handlers_.end()) return false;
  it->second();
  return true;
}

std::optional<std::string> CommandRegistry::CommandForChord(const ftxui::Event& event) const {
  for (const auto& binding : bindings_) {
    if (binding.chord == event) return binding.command;
  }
  return std::nullopt;
}

}  // namespace puka

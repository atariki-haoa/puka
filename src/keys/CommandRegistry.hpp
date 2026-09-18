#pragma once
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <ftxui/component/event.hpp>

#include "keys/KeyBinding.hpp"

namespace puka {

// Table-driven command dispatch: UI components look up "what command does
// this chord mean" and "run this command by name" here instead of scattering
// `if (event == Event::CtrlS)` checks across the codebase, so adding a
// binding later is a table edit, not a code change.
class CommandRegistry {
 public:
  explicit CommandRegistry(std::vector<Binding> bindings);

  void Register(const std::string& command, std::function<void()> handler);

  // Returns false if `command` has no registered handler (safe no-op).
  bool Dispatch(const std::string& command) const;

  std::optional<std::string> CommandForChord(const ftxui::Event& event) const;

  const std::vector<Binding>& Bindings() const { return bindings_; }

 private:
  std::vector<Binding> bindings_;
  std::unordered_map<std::string, std::function<void()>> handlers_;
};

}  // namespace puka

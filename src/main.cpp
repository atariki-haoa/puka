#include <filesystem>
#include <iostream>
#include <string>

#include "app/Application.hpp"
#include "git/GitService.hpp"
#include "ui/Icons.hpp"

int main(int argc, char** argv) {
  std::filesystem::path root;
  bool use_nerd_font = false;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--nerd-font") {
      use_nerd_font = true;
    } else {
      root = arg;
    }
  }
  if (root.empty()) root = std::filesystem::current_path();

  std::error_code ec;
  if (!std::filesystem::is_directory(root, ec)) {
    std::cerr << "puka: '" << root.string() << "' is not a directory\n";
    return 1;
  }

  // Off by default: renders as broken boxes/tofu unless the terminal's font
  // is actually a Nerd Font patched font (e.g. "FiraCode Nerd Font").
  puka::Icons::SetUseNerdFont(use_nerd_font);

  puka::InitGitLibrary();
  puka::Application app(std::filesystem::absolute(root, ec));
  int status = app.Run();
  puka::ShutdownGitLibrary();
  return status;
}

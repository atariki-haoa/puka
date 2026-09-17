#pragma once
#include <filesystem>
#include <memory>
#include <unordered_map>

#include <ftxui/component/screen_interactive.hpp>

#include "editor/DocumentManager.hpp"
#include "git/GitService.hpp"
#include "keys/CommandRegistry.hpp"
#include "ui/Layout.hpp"
#include "ui/Sidebar.hpp"
#include "ui/SourceControlView.hpp"

namespace puka {

class Application {
 public:
  explicit Application(std::filesystem::path workspace_root);
  int Run();

 private:
  void RegisterCommands();
  void RefreshGitStatus();

  std::filesystem::path workspace_root_;
  ftxui::ScreenInteractive screen_;
  DocumentManager documents_;
  CommandRegistry commands_;

  GitRepoStatus git_status_;
  std::unordered_map<std::filesystem::path, GitFileStatus> git_status_by_path_;
  std::shared_ptr<SourceControlView> source_control_;

  std::shared_ptr<Sidebar> sidebar_;
  ftxui::Component editor_;
  std::shared_ptr<Layout> layout_;
};

}  // namespace puka

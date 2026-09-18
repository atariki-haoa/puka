#pragma once
#include <atomic>
#include <condition_variable>
#include <filesystem>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>

#include <ftxui/component/screen_interactive.hpp>

#include "editor/DocumentManager.hpp"
#include "git/GitService.hpp"
#include "keys/CommandRegistry.hpp"
#include "ui/DiffView.hpp"
#include "ui/EditorView.hpp"
#include "ui/FileTreeView.hpp"
#include "ui/Layout.hpp"
#include "ui/ShortcutsPopup.hpp"
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
  void OpenDiff(const std::filesystem::path& path);

  // Wakes the otherwise purely input-driven FTXUI loop on an interval so git
  // status (and ahead/behind vs. upstream) stays live without a keypress --
  // see StartGitPollThread's doc comment in Application.cpp for why the
  // thread itself never touches libgit2.
  void StartGitPollThread();
  void StopGitPollThread();

  std::filesystem::path workspace_root_;
  ftxui::ScreenInteractive screen_;
  DocumentManager documents_;
  CommandRegistry commands_;

  std::thread git_poll_thread_;
  std::mutex git_poll_mutex_;
  std::condition_variable git_poll_cv_;
  std::atomic<bool> git_poll_stop_{false};

  GitRepoStatus git_status_;
  std::unordered_map<std::filesystem::path, GitFileStatus> git_status_by_path_;
  std::shared_ptr<SourceControlView> source_control_;

  std::shared_ptr<Sidebar> sidebar_;
  std::shared_ptr<EditorView> editor_view_;
  std::shared_ptr<FileTreeView> file_tree_view_;
  ftxui::Component editor_;
  std::shared_ptr<Layout> layout_;

  std::shared_ptr<DiffView> diff_view_;
  bool show_diff_ = false;

  bool show_shortcuts_ = false;
};

}  // namespace puka

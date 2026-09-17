#pragma once
#include <filesystem>
#include <functional>

#include <ftxui/component/component_base.hpp>

#include "git/GitService.hpp"

namespace puka {

class SourceControlView : public ftxui::ComponentBase {
 public:
  SourceControlView(std::filesystem::path root,
                     std::function<void(const std::filesystem::path&)> on_open,
                     std::function<void()> on_refresh_requested);

  ftxui::Element OnRender() override;
  bool OnEvent(ftxui::Event event) override;
  bool Focusable() const override { return true; }

  // Application is the sole caller of GetRepoStatus(); this is purely a
  // display sink so FileTreeView can share the same computed result
  // without it being computed twice per refresh.
  void SetStatus(GitRepoStatus status);

 private:
  std::filesystem::path root_;
  std::function<void(const std::filesystem::path&)> on_open_;
  std::function<void()> on_refresh_requested_;
  GitRepoStatus status_;
  int selected_ = 0;
};

}  // namespace puka

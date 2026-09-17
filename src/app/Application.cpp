#include "app/Application.hpp"

#include <algorithm>

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/terminal.hpp>

#include "keys/KeymapDefaults.hpp"
#include "search/SearchService.hpp"
#include "ui/EditorView.hpp"
#include "ui/FileTreeView.hpp"
#include "ui/SearchView.hpp"

namespace puka {
using namespace ftxui;

namespace {

Component PlaceholderView(std::string message) {
  return Renderer([message] {
    return vbox({filler(), hcenter(text(message) | dim), filler()});
  });
}

}  // namespace

Application::Application(std::filesystem::path workspace_root)
    : workspace_root_(std::move(workspace_root)),
      screen_(ScreenInteractive::Fullscreen()),
      commands_(DefaultKeymap()) {}

int Application::Run() {
  auto editor_view = Make<EditorView>(documents_);
  editor_ = editor_view;

  auto explorer = Make<FileTreeView>(
      workspace_root_,
      [this, editor_view](const std::filesystem::path& path) {
        if (documents_.OpenFile(path)) editor_view->TakeFocus();
      },
      &git_status_by_path_);

  auto search_view = Make<SearchView>(workspace_root_, [this, editor_view](const SearchHit& hit) {
    if (auto* doc = documents_.OpenFile(hit.file)) {
      size_t row = std::min(hit.line > 0 ? hit.line - 1 : 0, doc->buffer().LineCount() - 1);
      size_t col = std::min(hit.column > 0 ? hit.column - 1 : 0, doc->buffer().Line(row).size());
      doc->cursor().row = row;
      doc->cursor().col = col;
      editor_view->TakeFocus();
    }
  });

  auto source_control = Make<SourceControlView>(
      workspace_root_,
      [this, editor_view](const std::filesystem::path& path) {
        if (documents_.OpenFile(path)) editor_view->TakeFocus();
      },
      [this] { RefreshGitStatus(); });
  source_control_ = source_control;

  sidebar_ = Make<Sidebar>(explorer, search_view, source_control);
  RefreshGitStatus();  // populate before first paint, not just on first Alt+G

  int sidebar_width = std::max(20, Terminal::Size().dimx / 5);
  layout_ = Make<Layout>(sidebar_, editor_, &sidebar_width);

  RegisterCommands();

  Component root = CatchEvent(layout_, [this](Event event) {
    auto command = commands_.CommandForChord(event);
    return command.has_value() && commands_.Dispatch(*command);
  });

  // FTXUI's default Ctrl-Z handler otherwise wins even when a component
  // "catches" Event::CtrlZ -- verified empirically that force=false is what
  // lets our own Undo binding receive it instead.
  screen_.ForceHandleCtrlZ(false);

  screen_.Loop(root);
  return 0;
}

void Application::RegisterCommands() {
  commands_.Register("workbench.action.toggleSidebarVisibility", [this] {
    layout_->SetSidebarVisible(!layout_->SidebarVisible());
    if (!layout_->SidebarVisible()) editor_->TakeFocus();
  });
  commands_.Register("workbench.action.togglePaneFocus", [this] {
    if (sidebar_->Focused()) editor_->TakeFocus();
    else sidebar_->TakeFocus();
  });
  commands_.Register("workbench.view.explorer", [this] {
    sidebar_->SetActiveView(SidebarView::Explorer);
    sidebar_->TakeFocus();
  });
  commands_.Register("workbench.view.search", [this] {
    sidebar_->SetActiveView(SidebarView::Search);
    sidebar_->TakeFocus();
  });
  commands_.Register("workbench.view.scm", [this] {
    RefreshGitStatus();
    sidebar_->SetActiveView(SidebarView::SourceControl);
    sidebar_->TakeFocus();
  });
  commands_.Register("workbench.action.files.save", [this] {
    if (auto* doc = documents_.Active()) {
      if (doc->Save()) RefreshGitStatus();
    }
  });
  commands_.Register("workbench.action.closeActiveEditor", [this] { documents_.CloseActive(); });
  commands_.Register("workbench.action.nextEditor", [this] { documents_.NextTab(); });
  commands_.Register("workbench.action.previousEditor", [this] { documents_.PrevTab(); });
  commands_.Register("undo", [this] {
    if (auto* doc = documents_.Active()) doc->Undo();
  });
  commands_.Register("redo", [this] {
    if (auto* doc = documents_.Active()) doc->Redo();
  });
}

void Application::RefreshGitStatus() {
  git_status_ = GetRepoStatus(workspace_root_);
  git_status_by_path_.clear();
  for (const auto& f : git_status_.files) git_status_by_path_[f.path] = f;
  source_control_->SetStatus(git_status_);
}

}  // namespace puka

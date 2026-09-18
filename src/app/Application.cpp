#include "app/Application.hpp"

#include <algorithm>
#include <system_error>

#include <ftxui/component/component.hpp>
#include <ftxui/screen/terminal.hpp>

#include "keys/KeymapDefaults.hpp"
#include "search/SearchService.hpp"
#include "ui/EditorView.hpp"
#include "ui/FileTreeView.hpp"
#include "ui/SearchView.hpp"

namespace puka {
using namespace ftxui;

namespace {

// Translates the CommandRegistry's chord table into the popup's display
// list, merging fallback chords for the same command onto one row (e.g.
// Ctrl+W and Alt+W both close the active tab) so the popup doesn't show the
// same action twice.
std::vector<ShortcutEntry> BuildShortcutEntries(const std::vector<Binding>& bindings) {
  std::vector<ShortcutEntry> entries;
  std::unordered_map<std::string, size_t> index_by_command;
  for (const auto& binding : bindings) {
    auto [it, inserted] = index_by_command.emplace(binding.command, entries.size());
    if (inserted) {
      entries.push_back({binding.label, binding.description});
    } else {
      entries[it->second].label += " / " + binding.label;
    }
  }
  return entries;
}

std::string ShortcutsHint(const std::vector<Binding>& bindings) {
  for (const auto& binding : bindings) {
    if (binding.command == "workbench.action.toggleShortcutsHelp") {
      return binding.label + " " + binding.description;
    }
  }
  return "";
}

// Shortcuts handled directly inside a view's own OnEvent() (F5/t/Enter/
// Shift+Enter in Source Control, Escape in the diff view) rather than
// through CommandRegistry's global chord table, so BuildShortcutEntries()
// above never sees them. CLAUDE.md's keybinding rule requires every new
// shortcut -- global or local -- to show up in this popup, so this list is
// the manually-maintained half of that contract.
std::vector<ShortcutEntry> ContextualShortcutEntries() {
  return {
      {"F5", "Refresh git status (Source Control, focused)"},
      {"t", "Toggle Source Control List/Tree view"},
      {"Enter", "Open file diff vs. HEAD (Source Control)"},
      {"Shift+Enter / o", "Open file directly, no diff (Source Control)"},
      {"Esc", "Close the file diff view"},
  };
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
      &git_status_by_path_, &git_status_.ignored_paths);

  auto search_view = Make<SearchView>(workspace_root_, [this, editor_view](const SearchHit& hit) {
    if (auto* doc = documents_.OpenFile(hit.file)) {
      size_t row = std::min(hit.line > 0 ? hit.line - 1 : 0, doc->buffer().LineCount() - 1);
      size_t col = std::min(hit.column > 0 ? hit.column - 1 : 0, doc->buffer().Line(row).size());
      doc->cursor().row = row;
      doc->cursor().col = col;
      editor_view->TakeFocus();
    }
  });

  diff_view_ = Make<DiffView>(&show_diff_);

  auto source_control = Make<SourceControlView>(
      workspace_root_,
      [this, editor_view](const std::filesystem::path& path, ScmOpenMode mode) {
        if (mode == ScmOpenMode::Diff) {
          OpenDiff(path);
        } else if (documents_.OpenFile(path)) {
          editor_view->TakeFocus();
        }
      },
      [this] { RefreshGitStatus(); });
  source_control_ = source_control;

  sidebar_ = Make<Sidebar>(explorer, search_view, source_control);

  int sidebar_width = std::max(20, Terminal::Size().dimx / 5);
  layout_ = Make<Layout>(sidebar_, editor_, &sidebar_width, ShortcutsHint(commands_.Bindings()));
  RefreshGitStatus();  // populate before first paint, not just on first Alt+G

  auto shortcut_entries = BuildShortcutEntries(commands_.Bindings());
  auto contextual_entries = ContextualShortcutEntries();
  shortcut_entries.insert(shortcut_entries.end(), contextual_entries.begin(),
                           contextual_entries.end());
  auto shortcuts_popup = Make<ShortcutsPopup>(std::move(shortcut_entries), &show_shortcuts_);

  RegisterCommands();

  auto with_diff = Modal(layout_, diff_view_, &show_diff_);
  Component root = CatchEvent(Modal(with_diff, shortcuts_popup, &show_shortcuts_), [this](Event event) {
    // While the popup or the diff view is open, let it handle input
    // directly (each closes itself on Esc, the popup also on Enter/F1)
    // instead of letting global commands fire underneath it.
    if (show_shortcuts_ || show_diff_) return false;
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
  commands_.Register("workbench.action.nextSidebarView", [this] {
    sidebar_->CycleView(1);
    if (sidebar_->ActiveView() == SidebarView::SourceControl) RefreshGitStatus();
    sidebar_->TakeFocus();
  });
  commands_.Register("workbench.action.previousSidebarView", [this] {
    sidebar_->CycleView(-1);
    if (sidebar_->ActiveView() == SidebarView::SourceControl) RefreshGitStatus();
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
  commands_.Register("workbench.action.toggleShortcutsHelp",
                      [this] { show_shortcuts_ = !show_shortcuts_; });
}

void Application::RefreshGitStatus() {
  git_status_ = GetRepoStatus(workspace_root_);
  git_status_by_path_.clear();
  for (const auto& f : git_status_.files) git_status_by_path_[f.path] = f;
  source_control_->SetStatus(git_status_);
  layout_->SetGitStatus(git_status_);
}

void Application::OpenDiff(const std::filesystem::path& path) {
  std::error_code ec;
  auto rel = std::filesystem::relative(path, workspace_root_, ec);
  std::string title = ec ? path.string() : rel.string();
  diff_view_->SetDiff(title, GetFileDiff(workspace_root_, path));
  show_diff_ = true;
}

}  // namespace puka

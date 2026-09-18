# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

PUKA is a VSCode-like TUI code editor for Linux/macOS terminals, written in C++20 with
[FTXUI](https://github.com/ArthurSonzogni/FTXUI) (UI), [tree-sitter](https://github.com/tree-sitter/tree-sitter)
(syntax highlighting), and [libgit2](https://github.com/libgit2/libgit2) (git status). All three plus every
tree-sitter grammar are vendored via CMake `FetchContent` and statically linked — the only external
prerequisite is `cmake` itself.

## Build, run, test

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j"$(nproc)"

./build/src/puka [path]               # defaults to the current directory
./build/src/puka --nerd-font [path]   # graphical icons; requires a Nerd Font in the terminal

cd build && ctest --output-on-failure
ctest --test-dir build -R test_buffer_edits --output-on-failure   # run a single test
```

The first configure takes a few minutes (fetches/builds FTXUI, tree-sitter, grammars, libgit2); later
configures reuse the cache under `build/_deps`. `PUKA_BUILD_TESTS` (default `ON`) controls whether
`tests/` is built at all.

Each test in `tests/` is its own executable (`test_<name>` linking only the one library it exercises) —
see `tests/CMakeLists.txt` for the full list and which `puka_*` library each links.

## Architecture

### Module graph

The `src/` tree is one static library per subsystem (each with its own `CMakeLists.txt`), wired together
in `src/CMakeLists.txt`. Dependencies flow one way — lower layers know nothing about UI:

```
puka_fs      (FileTree)                         — no deps
puka_search  (SearchService)                    — no deps
puka_syntax  (LanguageRegistry, Highlighter,     — ftxui, tree-sitter core + grammars
              Theme)
puka_editor  (Buffer, Document,                 — puka_syntax
              DocumentManager)
puka_git     (GitService)                       — libgit2 (PRIVATE — no <git2/...> leaks past GitService.cpp)
puka_keys    (CommandRegistry, KeymapDefaults)  — ftxui
puka_ui      (Icons, FileTreeView, EditorView,  — puka_editor, puka_fs, puka_syntax, puka_search, puka_git
              SearchView, SourceControlView,
              ScmTree, DiffView, GitStatusBadge,
              Sidebar, Layout, StatusBar,
              ShortcutsPopup)
puka_app     (Application)                      — puka_ui, puka_editor, puka_keys, puka_fs, puka_search, puka_git
```

`main.cpp` only parses argv, calls `InitGitLibrary()`/`ShutdownGitLibrary()`, and hands off to
`Application`. `Application::Run()` (`src/app/Application.cpp`) is the composition root: it builds the
`FileTreeView`, `SearchView`, `SourceControlView`, wraps them in a `Sidebar`, pairs the sidebar with an
`EditorView` inside a `Layout`, registers commands, and drives the FTXUI event loop.

### Command dispatch (not scattered event checks)

Keybindings are table-driven through `CommandRegistry` (`src/keys/CommandRegistry.hpp`): a chord maps to a
command *name* (e.g. `"workbench.action.files.save"`), and handlers are registered separately by name in
`Application::RegisterCommands()`. `DefaultKeymap()` in `src/keys/KeymapDefaults.cpp` is the single table of
chord→command bindings, with comments explaining *why* each chord was chosen (terminals collapsing
`Ctrl+Shift+<letter>` to `Ctrl+<letter>`, tmux stealing `Ctrl+B`, terminals intercepting `Ctrl+W`, etc — see
that file and the README's Keybindings section before changing any binding). Adding a keybinding is a table
edit in `KeymapDefaults.cpp` plus a `Register()` call in `Application.cpp`, not a new `if (event == ...)`
branch.

Not every shortcut goes through this table — some are local to one view (e.g. Source Control's `F5`/`t`/
`Enter`/`Shift+Enter`, the diff view's `Esc`), handled directly inside that view's own `OnEvent()` because
they only make sense while it's focused. **Hard rule: every shortcut, global or local, must be discoverable
in the `F1` shortcuts popup — no exceptions.** A global `KeymapDefaults.cpp` binding gets this for free
(`Application::Run()` derives the popup from `commands_.Bindings()`); a local, view-scoped shortcut does
not, and must be added by hand to `ContextualShortcutEntries()` in `Application.cpp`. Adding or changing
either kind of shortcut without also updating the popup is an incomplete change.

### Editor core

- `Buffer` (`src/editor/Buffer.hpp`) is line-based, byte-oriented storage (not codepoint-aware — `col` is a
  byte offset) so it's tree-sitter-edit-compatible without a rewrite. It owns its own undo/redo stacks as
  `EditRecord`s and returns a `BufferEdit` delta from every mutation.
- `Document` (`src/editor/Document.hpp`) pairs a `Buffer` with a `Cursor` and an optional `Highlighter`
  (`nullptr` for unrecognized extensions — a clean no-op, not an error). `Document::Save()` writes to a temp
  file and renames over the original for atomicity.
- `DocumentManager` owns all open tabs. Returned `Document*` pointers are invalidated by any call that can
  add/remove documents (`OpenFile`/`CloseActive`) — re-fetch via `Active()` rather than caching across those
  calls.

### Syntax highlighting

`LanguageRegistry` (`src/syntax/LanguageRegistry.hpp`) is a deliberate Meyers singleton: it's a process-wide,
read-only table of `LanguageSpec`s (extension → tree-sitter grammar + highlights query) plus a cache of
compiled `TSQuery*`, reachable both from `Document::Open()`'s static factory and from anywhere else without
threading a reference through `DocumentManager::OpenFile(path)`. To add a language: add a grammar fetch in
the top-level `CMakeLists.txt` (following the `puka_add_ts_grammar` pattern — grammars are built by
compiling their checked-in `src/parser.c`/`scanner.c` directly, deliberately bypassing each grammar repo's
own CMakeLists.txt to avoid non-deterministic invocations of the `tree-sitter` CLI, which isn't installed),
add a highlights query header under `src/syntax/queries/`, and register a `LanguageSpec` in
`LanguageRegistry`'s constructor.

### Git status

`GitService` (`src/git/GitService.hpp`) is the *only* file that includes `<git2/...>` headers — the header
is libgit2-type-free, exposing only plain enums/structs, so nothing else in the codebase depends on libgit2
types. `GetRepoStatus()` is synchronous and stateless (reopens the repo every call — libgit2 doesn't cache
the untracked-directory walk across calls regardless of a kept-open handle, so there's no perf win to
holding one, only lifetime risk). Staged/unstaged deltas are independent per file (a file can be both). The
pure mapping functions (`MapStagedFlags`, `MapUnstagedFlags`, `BranchShorthand`) are separated out
specifically to be unit-testable without a live repository — see `tests/test_git_status_mapping.cpp`.
`GetFileDiff()` diffs one file's on-disk content against its blob at HEAD via libgit2's
`git_patch_from_blob_and_buffer` (no hand-rolled diff algorithm) — a missing HEAD blob (untracked file) or
missing on-disk file (staged deletion) is simply passed as an empty buffer on that side, so both edge cases
fall out of the same code path instead of needing special-casing.

### UI composition

Built on FTXUI's component model (`ftxui::ComponentBase`, `Make<T>()`). `Layout` wraps
`ResizableSplitLeft(sidebar, editor)` so the sidebar can be hidden (`Ctrl+B`) while its internal state
(scroll position, expanded folders) survives being hidden. `Sidebar` switches between `FileTreeView`,
`SearchView`, and `SourceControlView` by `SidebarView` enum, each of which independently calls back into
`Application` (open a file, refresh git status) via constructor-injected `std::function` callbacks rather
than reaching back up through a parent pointer.

### Icons

`Icons` (`src/ui/Icons.hpp`/`.cpp`) provides Nerd Font glyphs by default; `--no-nerd-font` switches to
plain ASCII-fallback glyphs via `Icons::SetUseNerdFont()`, set once in `main.cpp` before the `Application`
is constructed.

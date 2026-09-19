# PUKA

A VSCode-like TUI code editor for current Linux/macOS terminals, built in
C++20 with [FTXUI](https://github.com/ArthurSonzogni/FTXUI) (UI),
[tree-sitter](https://github.com/tree-sitter/tree-sitter) (syntax
highlighting) and [libgit2](https://github.com/libgit2/libgit2) (git status).

Full vibecoding made, born by the necesity to get a simple TUI code editor.

Enjoy!

## Status

A 20%/80% sidebar/editor split, an Explorer file tree with icons (Nerd Font
glyphs by default, `--no-nerd-font` for plain ASCII icons), multi-tab file
open/edit/save, a visible block cursor, line numbers, and a focus indicator
(the active pane's divider turns cyan).

**Git status** (`Alt+G`) shows the current branch and every changed file with
a two-letter porcelain-style badge (`A `/`M `/`??`/`MM`/etc. -- staged and
unstaged deltas are independent, so a file with both shows both), read-only
(no staging/committing). Matching files in the Explorer tree get the same
single-letter badge (unstaged wins when a file has both). Refreshes on
startup, on every `Alt+G`, after every save, and on `F5` while Source Control
is focused. Silently shows "Not a git repository" outside a git workspace.
Badges can get squeezed out by a long filename in a narrow sidebar -- resize
the terminal wider if one seems to be missing. Changed files default to a
flat List (full path per row); press `t` while Source Control is focused to
switch to a Tree grouped by folder (`Right`/`Left` or `Enter` on a folder
expand/collapse it there) -- a folder's collapsed state survives refreshes.
`Enter` on a *file* opens a full-pane, read-only diff against `HEAD`
(original left, working copy right, changed lines colored; `Esc` closes it);
`Shift+Enter` -- or `o`, since Shift+Enter isn't reliably distinguishable
from plain Enter in every terminal -- opens the file directly instead,
skipping the diff.

The bottom status bar always shows the repository name and current branch on
the left (blank outside a git workspace), plus a compact per-type change
count next to it when there are changes (e.g. `+3 ~2 ?1` for 3 added, 2
modified, 1 untracked) -- so you don't need to switch to Source Control just
to see whether you're clean or what's changed.

**Syntax highlighting** (VSCode Dark+-approximating colors) works for C, C++,
Python, JavaScript, TypeScript, TSX, JSON, and Bash, detected by file
extension (`.h`/`.hpp` resolve to C++). Unrecognized extensions open and edit
normally with no coloring. Markdown and additional languages (Rust, Go, ...)
are easy to add later following the same pattern in
`src/syntax/LanguageRegistry.cpp` but aren't wired up yet.

**Global search** (`Alt+F`) searches every file under the workspace root for
a plain-text query and jumps straight to the exact line/column on Enter. It
tries `rg` first (respects `.gitignore`, fastest), falls back to `grep`
(`--exclude-dir=.git`/`build`), and falls back further to a built-in
recursive scan if neither is installed -- the status line under the search
box says which one actually ran. Runs synchronously (may briefly block the
UI on a huge repo); results are capped at 500 with a "(truncated)" note
rather than silently dropping the rest. `ArrowDown` from the query box moves
into the results list, `ArrowUp` at the top result moves back.

## Install

```bash
git clone https://github.com/atariki-haoa/puka.git
cd puka
./install.sh
```

Builds a release binary in `./build` (reusing it, and its already-fetched
FTXUI/tree-sitter/libgit2 sources, if you'd already built puka there) and
installs it to `~/.local/bin/puka` (set `PREFIX=/usr/local` to install
system-wide instead, or `BUILD_DIR=...` to build somewhere other than
`./build`). Add `~/.local/bin` to your `PATH` if it isn't already. There are
no prebuilt binaries yet, so this still does a full source build under the
hood -- see Build below for what that entails. Since puka's icons default to
Nerd Font glyphs, the installer also checks whether a
[Nerd Font](https://www.nerdfonts.com/) is already installed and, if not,
offers to download and install one (JetBrainsMono Nerd Font) before
building -- remember to select it in your terminal profile's font settings
afterwards.

## Build

```bash
# Prerequisite: cmake (not required otherwise -- FTXUI is fetched and built
# from source, so no other system packages are needed).
sudo apt-get install -y cmake   # or: brew install cmake

cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j"$(nproc)"

# Optional: install the built binary (defaults to /usr/local/bin; override
# with -DCMAKE_INSTALL_PREFIX=... at configure time).
cmake --install build
```

The first configure takes a few minutes (fetches and builds FTXUI); later
configures are cached under `build/_deps`.

## Run

```bash
./build/src/puka [path]                  # defaults to the current directory
./build/src/puka --no-nerd-font [path]   # use plain text glyphs instead of graphical icons
```

By default puka uses [Nerd Font](https://www.nerdfonts.com/) glyphs, which
requires your terminal to actually be using a Nerd Font patched font (e.g.
"FiraCode Nerd Font", "JetBrainsMono Nerd Font") -- otherwise the icons
render as broken boxes/tofu characters. `install.sh` checks for one and
offers to install it for you; if you built from source manually and don't
have one, pass `--no-nerd-font` for plain ASCII icons instead. On Ubuntu,
`sudo apt install fonts-firacode` does not include the Nerd Font patch --
download a patched font from nerdfonts.com and select it in your terminal
profile's font settings first.

## Test

```bash
cd build && ctest --output-on-failure
```

## Versioning

The current version is the single `project(puka VERSION X.Y.Z ...)` line at the top of
`CMakeLists.txt`. The **Release** workflow (`.github/workflows/release.yml`) runs on every push
to `main` -- which includes every PR merged into `main`, whatever merge strategy is used -- and
looks at the *leading word* of the commit message that landed there -- whatever follows it
(`:`, `/`, a space, `(scope):`, or nothing at all) is ignored:

| Leading word          | Bump  |
| ---------------------- | ----- |
| `fix`, `patch`         | patch |
| `feature`, `feat`, `minor` | minor |
| `major`                | major |

So `fix: ...`, `fix/...`, `fix(scope): ...`, and `fix ...` are all equivalent. For a
squash-merged PR the message is the PR title; for a "create a merge commit" PR it's either the
PR title or the source branch name (e.g. a PR from branch `feature/foo` bumps minor even if the
PR title doesn't start with `feature`). Commits matching none of these -- the common case -- are
left alone: no bump, no tag, no release. When a bump is detected, the workflow updates
`CMakeLists.txt`, commits, pushes a `vX.Y.Z` tag, and publishes a GitHub Release with
auto-generated notes.

To force a specific bump regardless of the last commit message, run the workflow manually from
the Actions tab and pick `major`/`minor`/`patch` instead of `auto`.

## Keybindings (Phase 1)

| Action | Chord(s) |
|---|---|
| Quit puka | `Ctrl+C` |
| Toggle sidebar | `Ctrl+B` |
| Switch focus between sidebar and editor | `Escape` |
| Explorer (files) / Search / Source Control | `Alt+B` / `Alt+F` / `Alt+G` |
| Next / previous sidebar view | `Alt+Right` / `Alt+Left` |
| Save | `Ctrl+S` |
| Close tab | `Ctrl+W` or `Alt+W` |
| Next / previous tab | `Ctrl+Right` / `Ctrl+Left` |
| Go to tab 1-9 | `Alt+1` .. `Alt+9` |
| Undo / Redo | `Ctrl+Z` / `Ctrl+Y` |
| Move / select | Arrows, Home, End, PageUp, PageDown |
| Switch tab / sidebar view, open a file, expand a folder | Mouse click |
| Refresh git status (while Source Control is focused) | `F5` |
| Toggle Source Control List/Tree view (while focused) | `t` |
| Open file diff vs. HEAD (Source Control) | `Enter` |
| Open file directly, no diff (Source Control) | `Shift+Enter` or `o` |
| Close the file diff view | `Escape` |
| Show keyboard shortcuts | `F1` |

A few of these diverge from VSCode's literal defaults on purpose: terminals
collapse `Ctrl+Shift+<letter>` to the same byte sequence as `Ctrl+<letter>`,
so VSCode's `Ctrl+Shift+E/F/G` can't be told apart from `Ctrl+E/F/G` here --
`Alt+<letter>` is used instead. `Ctrl+W` is commonly intercepted by terminal
emulators/IDEs to close their own tab, so `Alt+W` is a fallback bound to the
same action (`Ctrl+B` currently has no such fallback -- it's tmux's prefix
key, so it won't reach puka under default tmux). If a chord doesn't seem to
reach puka at all, it's very likely your terminal (or an IDE's integrated
terminal) swallowing it before puka ever sees it -- `Escape` is the one
binding that's essentially never intercepted, which is why it's the
recommended way to move focus back to the sidebar. `Alt+Right`/`Alt+Left` are
raw xterm escape sequences rather than an FTXUI-named event (there isn't
one), so they're the least portable bindings here -- if they don't reach
puka in your terminal, `Alt+B`/`Alt+F`/`Alt+G` reach the same views
directly. `Shift+Enter` in Source Control is in the same boat -- most
terminals send the exact same bytes for `Enter` and `Shift+Enter` by
default, so `o` is a guaranteed-reliable fallback bound to the same "open
directly" action. `Alt+1`..`Alt+9` likewise stand in for VSCode's
`Ctrl+1`..`Ctrl+9`: most of those don't have a distinct C0 control code in
classic xterm encoding, and the couple that do (`Ctrl+3`, `Ctrl+8`) collide
with the bytes `Escape` and `Backspace` already send. New keybindings are added
table-by-table in `src/keys/KeymapDefaults.cpp` as features land (or, for a
shortcut local to one view rather than global, directly in that view's
`OnEvent()` -- either way it must also be added to the `F1` shortcuts
popup, see CLAUDE.md), matching VSCode's actual chord whenever the terminal
can deliver it.

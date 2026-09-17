# puka

A VSCode-like TUI code editor for current Linux/macOS terminals, built in
C++20 with [FTXUI](https://github.com/ArthurSonzogni/FTXUI) (UI),
[tree-sitter](https://github.com/tree-sitter/tree-sitter) (syntax
highlighting) and [libgit2](https://github.com/libgit2/libgit2) (git
integration, later phase).

## Status

A 20%/80% sidebar/editor split, an Explorer file tree with icons (ASCII
fallback by default, `--nerd-font` for graphical icons), multi-tab file
open/edit/save, a visible block cursor, line numbers, and a focus indicator
(the active pane's divider turns cyan). Git status is designed but not built
yet -- the Source Control sidebar tab still shows a "coming in a later
phase" placeholder.

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

## Build

```bash
# Prerequisite: cmake (not required otherwise -- FTXUI is fetched and built
# from source, so no other system packages are needed).
sudo apt-get install -y cmake   # or: brew install cmake

cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j"$(nproc)"
```

The first configure takes a few minutes (fetches and builds FTXUI); later
configures are cached under `build/_deps`.

## Run

```bash
./build/src/puka [path]               # defaults to the current directory
./build/src/puka --nerd-font [path]   # use graphical icons instead of text glyphs
```

`--nerd-font` requires your terminal to actually be using a
[Nerd Font](https://www.nerdfonts.com/) patched font (e.g. "FiraCode Nerd
Font", "JetBrainsMono Nerd Font") -- otherwise the icons render as broken
boxes/tofu characters. It's off by default (plain ASCII icons) for exactly
that reason. On Ubuntu, `sudo apt install fonts-firacode` does not include
the Nerd Font patch -- download a patched font from nerdfonts.com and select
it in your terminal profile's font settings first.

## Test

```bash
cd build && ctest --output-on-failure
```

## Keybindings (Phase 1)

| Action | Chord(s) |
|---|---|
| Quit puka | `Ctrl+C` |
| Toggle sidebar | `Ctrl+B` |
| Switch focus between sidebar and editor | `Escape` |
| Explorer (files) / Search / Source Control | `Alt+B` / `Alt+F` / `Alt+G` |
| Save | `Ctrl+S` |
| Close tab | `Ctrl+W` or `Alt+W` |
| Next / previous tab | `Ctrl+Right` / `Ctrl+Left` |
| Undo / Redo | `Ctrl+Z` / `Ctrl+Y` |
| Move / select | Arrows, Home, End, PageUp, PageDown |

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
recommended way to move focus back to the sidebar. New keybindings are added
table-by-table in `src/keys/KeymapDefaults.cpp` as features land, matching
VSCode's actual chord whenever the terminal can deliver it.

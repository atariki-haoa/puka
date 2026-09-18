#!/usr/bin/env bash
# Builds puka from source and installs it -- no prebuilt binaries exist yet,
# so this configures/builds with CMake (which itself fetches and statically
# links FTXUI, tree-sitter, and libgit2) and runs `cmake --install`.
#
# Run from inside an already-cloned checkout of the repo:
#   git clone https://github.com/atariki-haoa/puka.git
#   cd puka
#   ./install.sh
#
# Usage:
#   ./install.sh                 # installs to ~/.local
#   PREFIX=/usr/local ./install.sh   # install system-wide instead
#   BUILD_DIR=build-release ./install.sh   # build in a dir other than ./build
set -euo pipefail

PREFIX="${PREFIX:-$HOME/.local}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$SCRIPT_DIR"
BUILD_DIR="${BUILD_DIR:-$REPO_DIR/build}"

if [ ! -f "$REPO_DIR/CMakeLists.txt" ] || [ ! -d "$REPO_DIR/src" ]; then
  echo "error: install.sh must be run from inside a cloned puka checkout." >&2
  echo "  git clone https://github.com/atariki-haoa/puka.git && cd puka && ./install.sh" >&2
  exit 1
fi

# Only used for the optional Nerd Font zip download below -- not for cloning
# the repo, which is assumed to already be checked out.
TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

if ! command -v cmake >/dev/null 2>&1; then
  echo "error: cmake is required but was not found on PATH." >&2
  echo "Install it first (e.g. 'sudo apt-get install cmake' or 'brew install cmake')." >&2
  exit 1
fi
if ! command -v git >/dev/null 2>&1; then
  echo "error: git is required (CMake's FetchContent uses it to fetch FTXUI/tree-sitter/libgit2)." >&2
  exit 1
fi

# puka's icons default to Nerd Font glyphs (broken boxes/tofu without one),
# so check for one before building and offer to install one if missing.
NERD_FONT_NAME="JetBrainsMono"
FONT_DIRS=(
  "$HOME/.local/share/fonts"
  "$HOME/.fonts"
  "/usr/local/share/fonts"
  "/usr/share/fonts"
  "$HOME/Library/Fonts"
  "/Library/Fonts"
)

has_nerd_font() {
  if command -v fc-list >/dev/null 2>&1; then
    if fc-list | grep -qi "nerd font"; then
      return 0
    fi
  fi
  for dir in "${FONT_DIRS[@]}"; do
    [ -d "$dir" ] || continue
    if find "$dir" -iname "*nerd*font*" -print -quit 2>/dev/null | grep -q .; then
      return 0
    fi
  done
  return 1
}

install_nerd_font() {
  local target_dir="$HOME/.local/share/fonts"
  if [ "$(uname -s)" = "Darwin" ]; then
    target_dir="$HOME/Library/Fonts"
  fi
  mkdir -p "$target_dir"

  local zip_url="https://github.com/ryanoasis/nerd-fonts/releases/latest/download/${NERD_FONT_NAME}.zip"
  local zip_path="$TMP_DIR/${NERD_FONT_NAME}.zip"

  echo "==> Downloading ${NERD_FONT_NAME} Nerd Font"
  if ! curl -fsSL "$zip_url" -o "$zip_path"; then
    echo "warning: failed to download the Nerd Font -- continuing without it." >&2
    echo "You can install one manually from https://www.nerdfonts.com/ later, or run puka with --no-nerd-font." >&2
    return
  fi

  if ! command -v unzip >/dev/null 2>&1; then
    echo "warning: 'unzip' is required to install the font but was not found on PATH." >&2
    echo "Install it (e.g. 'sudo apt-get install unzip') and re-run, or run puka with --no-nerd-font." >&2
    return
  fi

  echo "==> Installing ${NERD_FONT_NAME} Nerd Font to $target_dir"
  unzip -oq "$zip_path" -d "$target_dir"

  if command -v fc-cache >/dev/null 2>&1; then
    fc-cache -f "$target_dir" >/dev/null 2>&1 || true
  fi

  echo "Font installed. Select \"${NERD_FONT_NAME} Nerd Font\" in your terminal's font settings to see the icons."
}

if has_nerd_font; then
  echo "==> Nerd Font detected -- puka's icons will render correctly."
else
  echo "==> No Nerd Font detected."
  echo "puka's icons default to Nerd Font glyphs and render as broken boxes/tofu without one."
  if [ -t 0 ]; then
    read -r -p "Install ${NERD_FONT_NAME} Nerd Font now? [Y/n] " reply || reply="n"
  else
    reply="n"
    echo "(non-interactive shell -- skipping; re-run interactively to be prompted, or install one manually)"
  fi
  case "$reply" in
    [nN]*)
      echo "Skipping font install. Run puka with --no-nerd-font for plain ASCII icons instead."
      ;;
    *)
      install_nerd_font
      ;;
  esac
fi

echo "==> Configuring in $BUILD_DIR (fetches and builds FTXUI, tree-sitter, and libgit2 on first run -- a few minutes; reused on later runs)"
cmake -S "$REPO_DIR" -B "$BUILD_DIR" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$PREFIX" \
  -DPUKA_BUILD_TESTS=OFF

echo "==> Building"
NPROC="$(command -v nproc >/dev/null 2>&1 && nproc || sysctl -n hw.ncpu)"
cmake --build "$BUILD_DIR" -j"$NPROC"

echo "==> Installing to $PREFIX/bin"
cmake --install "$BUILD_DIR"

echo
echo "puka installed: $PREFIX/bin/puka"
case ":$PATH:" in
  *":$PREFIX/bin:"*) ;;
  *)
    echo "NOTE: $PREFIX/bin is not on your PATH. Add this to your shell profile:"
    echo "  export PATH=\"$PREFIX/bin:\$PATH\""
    ;;
esac

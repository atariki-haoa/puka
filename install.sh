#!/usr/bin/env bash
# Builds puka from source and installs it -- no prebuilt binaries exist yet,
# so this clones the repo, configures/builds with CMake (which itself fetches
# and statically links FTXUI, tree-sitter, and libgit2), and runs
# `cmake --install`.
#
# Usage:
#   curl -fsSL https://raw.githubusercontent.com/atariki-haoa/puka/main/install.sh | bash
#   PREFIX=/usr/local ./install.sh   # install system-wide instead of ~/.local
set -euo pipefail

REPO_URL="https://github.com/atariki-haoa/puka.git"
PREFIX="${PREFIX:-$HOME/.local}"

if ! command -v cmake >/dev/null 2>&1; then
  echo "error: cmake is required but was not found on PATH." >&2
  echo "Install it first (e.g. 'sudo apt-get install cmake' or 'brew install cmake')." >&2
  exit 1
fi
if ! command -v git >/dev/null 2>&1; then
  echo "error: git is required but was not found on PATH." >&2
  exit 1
fi

WORKDIR="$(mktemp -d)"
trap 'rm -rf "$WORKDIR"' EXIT

echo "==> Cloning puka into $WORKDIR"
git clone --depth 1 "$REPO_URL" "$WORKDIR/puka"

echo "==> Configuring (fetches and builds FTXUI, tree-sitter, and libgit2 -- a few minutes on first run)"
cmake -S "$WORKDIR/puka" -B "$WORKDIR/puka/build" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$PREFIX" \
  -DPUKA_BUILD_TESTS=OFF

echo "==> Building"
NPROC="$(command -v nproc >/dev/null 2>&1 && nproc || sysctl -n hw.ncpu)"
cmake --build "$WORKDIR/puka/build" -j"$NPROC"

echo "==> Installing to $PREFIX/bin"
cmake --install "$WORKDIR/puka/build"

echo
echo "puka installed: $PREFIX/bin/puka"
case ":$PATH:" in
  *":$PREFIX/bin:"*) ;;
  *)
    echo "NOTE: $PREFIX/bin is not on your PATH. Add this to your shell profile:"
    echo "  export PATH=\"$PREFIX/bin:\$PATH\""
    ;;
esac

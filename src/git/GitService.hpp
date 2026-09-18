#pragma once
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace puka {

// Must be called exactly once before any other function below, and
// ShutdownGitLibrary() exactly once at process exit -- see main.cpp. Kept
// separate from GetRepoStatus (rather than lazy-init-on-first-call) so all
// libgit2 headers stay confined to GitService.cpp; nothing else in puka
// ever needs to see a <git2/...> include.
void InitGitLibrary();
void ShutdownGitLibrary();

enum class GitDeltaType { None, Added, Modified, Deleted, Renamed, TypeChange, Untracked };

// One entry per path libgit2's status scan reported. `staged` and
// `unstaged` are independent -- both may be non-None for the same path at
// once (e.g. a staged modification with further unstaged edits on top).
// `unstaged == Untracked` only ever appears with `staged == None`.
struct GitFileStatus {
  std::filesystem::path path;  // absolute
  GitDeltaType staged = GitDeltaType::None;
  GitDeltaType unstaged = GitDeltaType::None;
  bool conflicted = false;
};

struct GitRepoStatus {
  bool is_repo = false;  // false => `root` is not inside a git repo (or it
                          // couldn't be opened) -- the common, silent case
                          // for any non-git directory.
  std::string branch;     // empty if !is_repo
  bool detached = false;  // true => `branch` holds an abbreviated commit hash
  std::filesystem::path repo_root;  // absolute working directory; empty if
                                     // !is_repo or the repo is bare
  std::vector<GitFileStatus> files;

  // Paths (absolute) covered by a .gitignore rule (or .git/info/exclude).
  // An ignored directory is reported as a single entry for the directory
  // itself -- libgit2 doesn't recurse into it -- so a path nested inside one
  // of these needs IsPathIgnored's prefix check below, not an exact match.
  std::vector<std::filesystem::path> ignored_paths;
};

// Synchronous, stateless (reopens the repository every call -- libgit2 does
// not cache the untracked-directory walk across calls regardless of whether
// a git_repository handle stays open, so there is no real perf win to
// keeping one around, only lifetime-bug risk). Searches upward from `root`
// for a .git the way real git does. Never throws; returns is_repo=false
// silently on any failure (not a repo, unreadable, etc.).
GitRepoStatus GetRepoStatus(const std::filesystem::path& root);

enum class GitDiffLineOrigin { Context, Addition, Deletion };

// One line of a computed file diff. `old_lineno`/`new_lineno` are -1 when
// the line has no counterpart on that side (a pure addition has no
// old_lineno, a pure deletion has no new_lineno) -- context lines always
// have both.
struct GitDiffLine {
  GitDiffLineOrigin origin;
  int old_lineno = -1;
  int new_lineno = -1;
  std::string content;  // no trailing newline
};

struct GitFileDiff {
  bool available = false;  // false => repo/blob/file couldn't be read at all
  bool binary = false;     // true => content differs but is binary; `lines` is empty
  std::vector<GitDiffLine> lines;
};

// Diffs `path`'s current on-disk content against its blob at HEAD. A file
// absent from HEAD (untracked/new) diffs against an empty "old" side, so
// the whole file shows as additions; a file absent from disk (staged
// deletion) diffs against an empty "new" side, so it shows as all
// deletions. Synchronous, like GetRepoStatus.
GitFileDiff GetFileDiff(const std::filesystem::path& root, const std::filesystem::path& path);

// --- Pure logic below, unit-testable without a live repository ---------
// Take plain bitmasks/strings so this header stays libgit2-type-free; only
// GitService.cpp includes any <git2/...> header.
GitDeltaType MapStagedFlags(unsigned int raw);    // raw & GIT_STATUS_INDEX_*
GitDeltaType MapUnstagedFlags(unsigned int raw);  // raw & GIT_STATUS_WT_*

// "refs/heads/main" -> "main"; anything without that prefix (unexpected ref
// form) is returned unchanged.
std::string BranchShorthand(std::string_view symbolic_target);

// True if `path` is itself one of `ignored`, or nested inside one of them
// (see GitRepoStatus::ignored_paths for why a prefix check -- not just an
// exact match -- is needed).
bool IsPathIgnored(const std::filesystem::path& path,
                    const std::vector<std::filesystem::path>& ignored);

}  // namespace puka

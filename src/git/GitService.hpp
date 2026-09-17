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
  std::vector<GitFileStatus> files;
};

// Synchronous, stateless (reopens the repository every call -- libgit2 does
// not cache the untracked-directory walk across calls regardless of whether
// a git_repository handle stays open, so there is no real perf win to
// keeping one around, only lifetime-bug risk). Searches upward from `root`
// for a .git the way real git does. Never throws; returns is_repo=false
// silently on any failure (not a repo, unreadable, etc.).
GitRepoStatus GetRepoStatus(const std::filesystem::path& root);

// --- Pure logic below, unit-testable without a live repository ---------
// Take plain bitmasks/strings so this header stays libgit2-type-free; only
// GitService.cpp includes any <git2/...> header.
GitDeltaType MapStagedFlags(unsigned int raw);    // raw & GIT_STATUS_INDEX_*
GitDeltaType MapUnstagedFlags(unsigned int raw);  // raw & GIT_STATUS_WT_*

// "refs/heads/main" -> "main"; anything without that prefix (unexpected ref
// form) is returned unchanged.
std::string BranchShorthand(std::string_view symbolic_target);

}  // namespace puka

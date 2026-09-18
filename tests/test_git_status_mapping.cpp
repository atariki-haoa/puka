#include "git/GitService.hpp"
#include "test_util.hpp"

using namespace puka;
using namespace puka::test;

namespace {

// Literal bit values mirror libgit2's git_status_t enum (verified against
// include/git2/status.h): these values are part of libgit2's stable public
// ABI, so hardcoding them here (rather than including <git2/status.h>) keeps
// this test -- like GitService.hpp itself -- fully libgit2-type-free.
constexpr unsigned kIndexNew        = 1u << 0;  // GIT_STATUS_INDEX_NEW
constexpr unsigned kIndexModified   = 1u << 1;  // GIT_STATUS_INDEX_MODIFIED
constexpr unsigned kIndexDeleted    = 1u << 2;  // GIT_STATUS_INDEX_DELETED
constexpr unsigned kIndexRenamed    = 1u << 3;  // GIT_STATUS_INDEX_RENAMED
constexpr unsigned kWtNew           = 1u << 7;  // GIT_STATUS_WT_NEW
constexpr unsigned kWtModified      = 1u << 8;  // GIT_STATUS_WT_MODIFIED
constexpr unsigned kWtDeleted       = 1u << 9;  // GIT_STATUS_WT_DELETED

void TestMapStagedFlags() {
  Check(MapStagedFlags(kIndexNew) == GitDeltaType::Added, "staged new -> Added");
  Check(MapStagedFlags(kIndexModified) == GitDeltaType::Modified, "staged modified");
  Check(MapStagedFlags(kIndexDeleted) == GitDeltaType::Deleted, "staged deleted");
  Check(MapStagedFlags(kIndexRenamed) == GitDeltaType::Renamed, "staged renamed");
  Check(MapStagedFlags(0) == GitDeltaType::None, "no staged flags -> None");
}

void TestMapUnstagedFlags() {
  Check(MapUnstagedFlags(kWtNew) == GitDeltaType::Untracked,
        "WT_NEW maps to Untracked, not Added (a workdir-only new file is untracked)");
  Check(MapUnstagedFlags(kWtModified) == GitDeltaType::Modified, "unstaged modified");
  Check(MapUnstagedFlags(kWtDeleted) == GitDeltaType::Deleted, "unstaged deleted");
  Check(MapUnstagedFlags(0) == GitDeltaType::None, "no unstaged flags -> None");
}

void TestMapFlagsHandleCombinedBits() {
  // A file staged-modified AND further modified in the worktree on top --
  // each mapper only looks at its own INDEX_*/WT_* bit range.
  unsigned combined = kIndexModified | kWtModified;
  Check(MapStagedFlags(combined) == GitDeltaType::Modified, "combined: staged side still Modified");
  Check(MapUnstagedFlags(combined) == GitDeltaType::Modified, "combined: unstaged side still Modified");
}

void TestBranchShorthandStripsPrefix() {
  Check(BranchShorthand("refs/heads/main") == "main", "strips refs/heads/ prefix");
  Check(BranchShorthand("refs/heads/feature/x") == "feature/x",
        "strips prefix even when the branch name itself contains slashes");
}

void TestBranchShorthandPassesThroughUnknownForm() {
  Check(BranchShorthand("refs/tags/v1") == "refs/tags/v1", "non-branch ref form left unchanged");
  Check(BranchShorthand("") == "", "empty input left unchanged");
}

void TestIsPathIgnored() {
  std::vector<std::filesystem::path> ignored = {"/repo/build", "/repo/.env"};
  Check(IsPathIgnored("/repo/build", ignored), "the ignored entry itself");
  Check(IsPathIgnored("/repo/build/sub/file.o", ignored),
        "nested under an ignored directory that libgit2 folded into one entry");
  Check(IsPathIgnored("/repo/.env", ignored), "exact-match ignored file");
  Check(!IsPathIgnored("/repo/src/build_helpers.cpp", ignored),
        "sibling that merely shares a path prefix, not nested under it");
  Check(!IsPathIgnored("/repo/src/main.cpp", ignored), "unrelated tracked file");
  Check(!IsPathIgnored("/repo", {}), "empty ignored list");
}

}  // namespace

int main() {
  TestMapStagedFlags();
  TestMapUnstagedFlags();
  TestMapFlagsHandleCombinedBits();
  TestBranchShorthandStripsPrefix();
  TestBranchShorthandPassesThroughUnknownForm();
  TestIsPathIgnored();

  if (g_failures == 0) {
    std::cout << "test_git_status_mapping: all tests passed\n";
    return 0;
  }
  std::cerr << "test_git_status_mapping: " << g_failures << " failure(s)\n";
  return 1;
}

#include "git/GitService.hpp"

#include <system_error>

#include <git2/global.h>
#include <git2/oid.h>
#include <git2/refs.h>
#include <git2/repository.h>
#include <git2/status.h>

namespace puka {

void InitGitLibrary() { git_libgit2_init(); }
void ShutdownGitLibrary() { git_libgit2_shutdown(); }

GitDeltaType MapStagedFlags(unsigned int raw) {
  if (raw & GIT_STATUS_INDEX_RENAMED) return GitDeltaType::Renamed;
  if (raw & GIT_STATUS_INDEX_NEW) return GitDeltaType::Added;
  if (raw & GIT_STATUS_INDEX_DELETED) return GitDeltaType::Deleted;
  if (raw & GIT_STATUS_INDEX_TYPECHANGE) return GitDeltaType::TypeChange;
  if (raw & GIT_STATUS_INDEX_MODIFIED) return GitDeltaType::Modified;
  return GitDeltaType::None;
}

GitDeltaType MapUnstagedFlags(unsigned int raw) {
  if (raw & GIT_STATUS_WT_RENAMED) return GitDeltaType::Renamed;
  if (raw & GIT_STATUS_WT_NEW) return GitDeltaType::Untracked;
  if (raw & GIT_STATUS_WT_DELETED) return GitDeltaType::Deleted;
  if (raw & GIT_STATUS_WT_TYPECHANGE) return GitDeltaType::TypeChange;
  if (raw & GIT_STATUS_WT_MODIFIED) return GitDeltaType::Modified;
  return GitDeltaType::None;
}

std::string BranchShorthand(std::string_view symbolic_target) {
  constexpr std::string_view kPrefix = "refs/heads/";
  if (symbolic_target.substr(0, kPrefix.size()) == kPrefix) {
    return std::string(symbolic_target.substr(kPrefix.size()));
  }
  return std::string(symbolic_target);
}

namespace {

const char* PathOf(const git_status_entry* entry) {
  if (entry->index_to_workdir) {
    if (entry->index_to_workdir->new_file.path) return entry->index_to_workdir->new_file.path;
    if (entry->index_to_workdir->old_file.path) return entry->index_to_workdir->old_file.path;
  }
  if (entry->head_to_index) {
    if (entry->head_to_index->new_file.path) return entry->head_to_index->new_file.path;
    if (entry->head_to_index->old_file.path) return entry->head_to_index->old_file.path;
  }
  return nullptr;
}

}  // namespace

GitRepoStatus GetRepoStatus(const std::filesystem::path& root) {
  GitRepoStatus result;

  std::error_code ec;
  std::string open_path = std::filesystem::absolute(root, ec).string();
  if (ec) open_path = root.string();

  git_repository* repo = nullptr;
  // flags=0, ceiling_dirs=nullptr: searches upward across parent
  // directories like real `git`, stopping at filesystem boundaries.
  if (git_repository_open_ext(&repo, open_path.c_str(), 0, nullptr) != 0) {
    return result;  // is_repo stays false -- not a git repo, silently
  }

  git_reference* head_ref = nullptr;
  if (git_reference_lookup(&head_ref, repo, "HEAD") == 0) {
    if (const char* symbolic = git_reference_symbolic_target(head_ref)) {
      result.branch = BranchShorthand(symbolic);
      result.detached = false;
    } else if (const git_oid* oid = git_reference_target(head_ref)) {
      char buf[9] = {};
      git_oid_tostr(buf, sizeof(buf), oid);
      result.branch = buf;
      result.detached = true;
    }
    git_reference_free(head_ref);
  }

  if (const char* workdir_cstr = git_repository_workdir(repo)) {
    std::string workdir = workdir_cstr;

    git_status_options opts = GIT_STATUS_OPTIONS_INIT;
    opts.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
    // Deliberately not GIT_STATUS_OPT_INCLUDE_IGNORED (matches VSCode's
    // default of not cluttering the view with ignored files) and not
    // GIT_STATUS_OPT_UPDATE_INDEX (mutates the on-disk index as a side
    // effect -- unnecessary and out of place for a read-only feature).
    opts.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED | GIT_STATUS_OPT_RECURSE_UNTRACKED_DIRS |
                 GIT_STATUS_OPT_RENAMES_HEAD_TO_INDEX;

    git_status_list* list = nullptr;
    if (git_status_list_new(&list, repo, &opts) == 0) {
      size_t count = git_status_list_entrycount(list);
      result.files.reserve(count);
      for (size_t i = 0; i < count; ++i) {
        const git_status_entry* entry = git_status_byindex(list, i);
        const char* rel = PathOf(entry);
        if (!rel) continue;
        GitFileStatus file;
        file.path = std::filesystem::path(workdir) / rel;
        file.staged = MapStagedFlags(static_cast<unsigned int>(entry->status));
        file.unstaged = MapUnstagedFlags(static_cast<unsigned int>(entry->status));
        file.conflicted = (entry->status & GIT_STATUS_CONFLICTED) != 0;
        result.files.push_back(std::move(file));
      }
      git_status_list_free(list);
    }
  }

  result.is_repo = true;
  git_repository_free(repo);
  return result;
}

}  // namespace puka

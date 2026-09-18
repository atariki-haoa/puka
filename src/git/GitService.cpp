#include "git/GitService.hpp"

#include <fstream>
#include <sstream>
#include <system_error>

#include <git2/blob.h>
#include <git2/global.h>
#include <git2/object.h>
#include <git2/oid.h>
#include <git2/patch.h>
#include <git2/refs.h>
#include <git2/repository.h>
#include <git2/revparse.h>
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

std::string StripTrailingNewline(std::string line) {
  if (!line.empty() && line.back() == '\n') line.pop_back();
  if (!line.empty() && line.back() == '\r') line.pop_back();
  return line;
}

GitDiffLineOrigin MapLineOrigin(char origin) {
  switch (origin) {
    case GIT_DIFF_LINE_ADDITION: return GitDiffLineOrigin::Addition;
    case GIT_DIFF_LINE_DELETION: return GitDiffLineOrigin::Deletion;
    default: return GitDiffLineOrigin::Context;
  }
}

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

  if (const char* workdir_cstr = git_repository_workdir(repo)) {
    result.repo_root = workdir_cstr;
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

  if (!result.repo_root.empty()) {
    // Zero-init (not the GIT_STATUS_OPTIONS_INIT macro, which only sets
    // `version` and triggers -Wmissing-field-initializers under -Wextra)
    // then set version manually, per git_status_options_init's own contract.
    git_status_options opts{};
    opts.version = GIT_STATUS_OPTIONS_VERSION;
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
        file.path = result.repo_root / rel;
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

GitFileDiff GetFileDiff(const std::filesystem::path& root, const std::filesystem::path& path) {
  GitFileDiff result;

  std::error_code ec;
  std::string open_path = std::filesystem::absolute(root, ec).string();
  if (ec) open_path = root.string();

  git_repository* repo = nullptr;
  if (git_repository_open_ext(&repo, open_path.c_str(), 0, nullptr) != 0) {
    return result;  // available stays false -- not a git repo
  }

  auto rel = std::filesystem::relative(path, root, ec);
  std::string rel_str = ec ? path.filename().string() : rel.generic_string();

  // "HEAD:<path>" is git's revspec syntax for "this path's blob as of this
  // commit" -- fails (object stays null) for a file that's untracked or
  // didn't exist yet at HEAD, which is exactly when we want an empty old
  // side below.
  git_object* head_obj = nullptr;
  git_blob* old_blob = nullptr;
  std::string revspec = "HEAD:" + rel_str;
  if (git_revparse_single(&head_obj, repo, revspec.c_str()) == 0) {
    if (git_object_type(head_obj) == GIT_OBJECT_BLOB) {
      old_blob = reinterpret_cast<git_blob*>(head_obj);
    } else {
      git_object_free(head_obj);
      head_obj = nullptr;
    }
  }

  // Missing from disk (e.g. a staged deletion) reads as an empty "new"
  // side below, same idea as the empty "old" side for an untracked file.
  std::ifstream in(path, std::ios::binary);
  std::string new_content;
  if (in) {
    std::ostringstream ss;
    ss << in.rdbuf();
    new_content = ss.str();
  }

  git_patch* patch = nullptr;
  int rc = git_patch_from_blob_and_buffer(&patch, old_blob, rel_str.c_str(), new_content.data(),
                                           new_content.size(), rel_str.c_str(), nullptr);
  if (rc == 0 && patch) {
    result.available = true;
    const git_diff_delta* delta = git_patch_get_delta(patch);
    if (delta && (delta->flags & GIT_DIFF_FLAG_BINARY)) {
      result.binary = true;
    } else {
      size_t hunk_count = git_patch_num_hunks(patch);
      for (size_t h = 0; h < hunk_count; ++h) {
        const git_diff_hunk* hunk = nullptr;
        size_t line_count = 0;
        if (git_patch_get_hunk(&hunk, &line_count, patch, h) != 0) continue;
        for (size_t l = 0; l < line_count; ++l) {
          const git_diff_line* line = nullptr;
          if (git_patch_get_line_in_hunk(&line, patch, h, l) != 0) continue;
          if (line->origin != GIT_DIFF_LINE_CONTEXT && line->origin != GIT_DIFF_LINE_ADDITION &&
              line->origin != GIT_DIFF_LINE_DELETION) {
            continue;  // EOFNL markers etc. -- not a real content line
          }
          GitDiffLine out_line;
          out_line.origin = MapLineOrigin(static_cast<char>(line->origin));
          out_line.old_lineno = line->old_lineno;
          out_line.new_lineno = line->new_lineno;
          out_line.content = StripTrailingNewline(std::string(line->content, line->content_len));
          result.lines.push_back(std::move(out_line));
        }
      }
    }
    git_patch_free(patch);
  }

  if (head_obj) git_object_free(head_obj);
  git_repository_free(repo);
  return result;
}

}  // namespace puka

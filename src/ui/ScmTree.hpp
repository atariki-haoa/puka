#pragma once
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "git/GitService.hpp"

namespace puka {

struct ScmTreeNode {
  std::string name;
  std::filesystem::path path;  // absolute
  bool is_directory = false;
  bool expanded = true;
  GitFileStatus status;  // only meaningful when !is_directory
  std::vector<std::unique_ptr<ScmTreeNode>> children;
};

// Synthetic folder tree built from a flat git-status file list for Source
// Control's "Tree" display mode -- unlike FileTree, nothing is read from
// disk; only paths that appear in the status list exist as nodes. Rebuilt
// wholesale on every SetFiles() call since Application::RefreshGitStatus()
// always replaces the full status list anyway; a folder's collapsed state
// survives the rebuild by path so refreshing doesn't reset what the user
// had folded.
class ScmTree {
 public:
  struct VisibleRow {
    ScmTreeNode* node;
    int depth;
  };

  void SetFiles(const std::vector<GitFileStatus>& files, const std::filesystem::path& root);

  // Flattens the currently-expanded nodes into display order (the synthetic
  // root itself is not included as a row).
  std::vector<VisibleRow> VisibleRows();

  void ToggleExpanded(ScmTreeNode& node);

 private:
  std::unique_ptr<ScmTreeNode> root_;
  std::unordered_set<std::filesystem::path> collapsed_;  // folders the user folded
};

}  // namespace puka

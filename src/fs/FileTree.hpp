#pragma once
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace puka {

struct FileTreeNode {
  std::filesystem::path path;
  std::string name;
  bool is_directory = false;
  bool expanded = false;
  bool children_loaded = false;
  std::vector<std::unique_ptr<FileTreeNode>> children;
};

// Lazily-expanding filesystem tree for the Explorer sidebar. Directory
// contents are only read from disk the first time a directory is expanded.
class FileTree {
 public:
  explicit FileTree(std::filesystem::path root);

  struct VisibleRow {
    FileTreeNode* node;
    int depth;
  };

  // Flattens the currently-expanded nodes into display order (the root
  // itself is not included as a row).
  std::vector<VisibleRow> VisibleRows();

  void ToggleExpanded(FileTreeNode& node);

 private:
  void EnsureChildrenLoaded(FileTreeNode& node);
  void CollectVisible(FileTreeNode& node, int depth, std::vector<VisibleRow>& out);

  std::unique_ptr<FileTreeNode> root_;
};

}  // namespace puka

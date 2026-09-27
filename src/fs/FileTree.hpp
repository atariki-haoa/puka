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
  // Stamped whenever `children` is (re)loaded (see
  // FileTree::EnsureChildrenLoaded) -- FileTree::RefreshExternalChanges()
  // compares this against the directory's current on-disk mtime to detect a
  // file/folder created or removed outside puka.
  std::filesystem::file_time_type known_mtime{};
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

  // The synthetic root node (the workspace directory itself) -- not a row in
  // VisibleRows(), but needed as a fallback "create here" target when
  // nothing in the tree is selected.
  FileTreeNode& Root() { return *root_; }

  // Re-reads `node`'s own directory listing from disk (e.g. after creating a
  // file inside it), while preserving each existing subdirectory's expanded
  // state and already-loaded children -- otherwise every subfolder under
  // `node` would silently re-collapse on refresh.
  void RefreshChildren(FileTreeNode& node);

  // Walks every expanded, already-loaded directory (recursively, including
  // the synthetic root, which is always expanded+loaded from construction)
  // and RefreshChildren()s any whose on-disk mtime moved since it was last
  // loaded -- e.g. an `mkdir`/file creation from outside puka. A directory's
  // own mtime changes exactly when a direct child is added/removed/renamed
  // (POSIX semantics), which is exactly what this needs to catch. Returns
  // true if anything changed, so the caller knows to recompute
  // VisibleRows()/selection.
  bool RefreshExternalChanges();

 private:
  void EnsureChildrenLoaded(FileTreeNode& node);
  void CollectVisible(FileTreeNode& node, int depth, std::vector<VisibleRow>& out);
  bool RefreshExternalChangesRecursive(FileTreeNode& node);

  std::unique_ptr<FileTreeNode> root_;
};

}  // namespace puka

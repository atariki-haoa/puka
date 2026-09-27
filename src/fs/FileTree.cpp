#include "fs/FileTree.hpp"

#include <algorithm>
#include <unordered_map>

namespace puka {

namespace {

std::unique_ptr<FileTreeNode> MakeNode(const std::filesystem::path& path) {
  auto node = std::make_unique<FileTreeNode>();
  node->path = path;
  node->name = path.filename().empty() ? path.string() : path.filename().string();
  std::error_code ec;
  node->is_directory = std::filesystem::is_directory(path, ec);
  return node;
}

}  // namespace

FileTree::FileTree(std::filesystem::path root) {
  std::error_code ec;
  root_ = MakeNode(std::filesystem::absolute(root, ec));
  root_->expanded = true;
  EnsureChildrenLoaded(*root_);
}

void FileTree::EnsureChildrenLoaded(FileTreeNode& node) {
  if (node.children_loaded || !node.is_directory) return;
  node.children_loaded = true;

  std::error_code ec;
  auto it = std::filesystem::directory_iterator(
      node.path, std::filesystem::directory_options::skip_permission_denied, ec);
  for (const auto& entry : it) {
    if (entry.path().filename() == ".git") continue;  // Phase 1: skip .git noise in the tree
    node.children.push_back(MakeNode(entry.path()));
  }

  std::sort(node.children.begin(), node.children.end(), [](const auto& a, const auto& b) {
    if (a->is_directory != b->is_directory) return a->is_directory;
    return a->name < b->name;
  });

  // Stamped right after the listing above so RefreshExternalChanges()'s very
  // next comparison reflects this load, not a stale/default value -- this is
  // the one call site both the initial load and every RefreshChildren() (via
  // its own call into this function) funnel through.
  std::error_code mtime_ec;
  node.known_mtime = std::filesystem::last_write_time(node.path, mtime_ec);
}

void FileTree::ToggleExpanded(FileTreeNode& node) {
  if (!node.is_directory) return;
  node.expanded = !node.expanded;
  if (node.expanded) EnsureChildrenLoaded(node);
}

void FileTree::RefreshChildren(FileTreeNode& node) {
  if (!node.is_directory) return;

  // Detach existing subdirectory nodes by name before rebuilding this
  // level's listing, so newly-created nodes of the same name can inherit
  // their expanded/children_loaded state and already-loaded subtree instead
  // of resetting to collapsed.
  std::unordered_map<std::string, std::unique_ptr<FileTreeNode>> old_dirs;
  for (auto& child : node.children) {
    if (child->is_directory) old_dirs[child->name] = std::move(child);
  }

  node.children_loaded = false;
  node.children.clear();
  EnsureChildrenLoaded(node);

  for (auto& child : node.children) {
    if (!child->is_directory) continue;
    auto it = old_dirs.find(child->name);
    if (it == old_dirs.end()) continue;
    child->expanded = it->second->expanded;
    child->children_loaded = it->second->children_loaded;
    child->children = std::move(it->second->children);
  }
}

void FileTree::CollectVisible(FileTreeNode& node, int depth, std::vector<VisibleRow>& out) {
  out.push_back({&node, depth});
  if (node.is_directory && node.expanded) {
    for (auto& child : node.children) CollectVisible(*child, depth + 1, out);
  }
}

std::vector<FileTree::VisibleRow> FileTree::VisibleRows() {
  std::vector<VisibleRow> out;
  for (auto& child : root_->children) CollectVisible(*child, 0, out);
  return out;
}

bool FileTree::RefreshExternalChanges() { return RefreshExternalChangesRecursive(*root_); }

bool FileTree::RefreshExternalChangesRecursive(FileTreeNode& node) {
  if (!node.is_directory || !node.expanded || !node.children_loaded) return false;

  std::error_code ec;
  auto mtime = std::filesystem::last_write_time(node.path, ec);
  bool changed = false;
  if (!ec && mtime != node.known_mtime) {
    // Rebuilds `node.children` (RefreshChildren -> EnsureChildrenLoaded also
    // re-stamps known_mtime), so the loop below always recurses into the
    // fresh listing, never a stale one.
    RefreshChildren(node);
    changed = true;
  }

  for (auto& child : node.children) {
    changed |= RefreshExternalChangesRecursive(*child);
  }
  return changed;
}

}  // namespace puka

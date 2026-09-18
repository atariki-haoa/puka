#include "ui/ScmTree.hpp"

#include <algorithm>
#include <system_error>

namespace puka {

namespace {

void SortChildren(ScmTreeNode& node) {
  std::sort(node.children.begin(), node.children.end(), [](const auto& a, const auto& b) {
    if (a->is_directory != b->is_directory) return a->is_directory;
    return a->name < b->name;
  });
  for (auto& child : node.children) {
    if (child->is_directory) SortChildren(*child);
  }
}

ScmTreeNode* FindOrCreateChild(ScmTreeNode& parent, const std::string& name, bool is_directory,
                                const std::filesystem::path& full_path,
                                const std::unordered_set<std::filesystem::path>& collapsed) {
  for (auto& child : parent.children) {
    if (child->name == name && child->is_directory == is_directory) return child.get();
  }
  auto node = std::make_unique<ScmTreeNode>();
  node->name = name;
  node->path = full_path;
  node->is_directory = is_directory;
  node->expanded = !collapsed.count(full_path);
  ScmTreeNode* raw = node.get();
  parent.children.push_back(std::move(node));
  return raw;
}

void CollectVisible(ScmTreeNode& node, int depth, std::vector<ScmTree::VisibleRow>& out) {
  out.push_back({&node, depth});
  if (node.is_directory && node.expanded) {
    for (auto& child : node.children) CollectVisible(*child, depth + 1, out);
  }
}

}  // namespace

void ScmTree::SetFiles(const std::vector<GitFileStatus>& files, const std::filesystem::path& root) {
  root_ = std::make_unique<ScmTreeNode>();
  root_->is_directory = true;
  root_->path = root;
  root_->expanded = true;

  for (const auto& file : files) {
    std::error_code ec;
    auto rel = std::filesystem::relative(file.path, root, ec);
    bool outside_root = ec || rel.empty() || *rel.begin() == "..";
    if (outside_root) {
      // e.g. the workspace was opened on a subdirectory of the repo, so
      // this changed file lives outside it -- show it as a single flat
      // leaf rather than guessing a folder structure that doesn't
      // correspond to the open workspace.
      ScmTreeNode* leaf =
          FindOrCreateChild(*root_, file.path.string(), false, file.path, collapsed_);
      leaf->status = file;
      continue;
    }

    ScmTreeNode* cursor = root_.get();
    std::filesystem::path built = root;
    for (auto it = rel.begin(); it != rel.end();) {
      std::string component = it->string();
      built /= component;
      ++it;
      bool is_last = (it == rel.end());
      cursor = FindOrCreateChild(*cursor, component, !is_last, built, collapsed_);
      if (is_last) cursor->status = file;
    }
  }

  SortChildren(*root_);
}

void ScmTree::ToggleExpanded(ScmTreeNode& node) {
  if (!node.is_directory) return;
  node.expanded = !node.expanded;
  if (node.expanded) {
    collapsed_.erase(node.path);
  } else {
    collapsed_.insert(node.path);
  }
}

std::vector<ScmTree::VisibleRow> ScmTree::VisibleRows() {
  std::vector<VisibleRow> out;
  if (!root_) return out;
  for (auto& child : root_->children) CollectVisible(*child, 0, out);
  return out;
}

}  // namespace puka

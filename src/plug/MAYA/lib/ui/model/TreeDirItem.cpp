#include <lib/ui/model/TreeDirItem.h>
namespace doodle::motion::ui {
TreeDirItem::TreeDirItem()
    : p_dir(),
      p_parent() {
}

TreeDirItem::TreeDirItem(std::string dir, TreeDirItemPtr parent)
    : p_dir(std::move(dir)),
      p_parent() {
  if (parent) {
    p_parent = parent;
    parent->p_child_items.push_back(shared_from_this());
  }
}

const std::string& TreeDirItem::Dir() const noexcept {
  return p_dir;
}

void TreeDirItem::setDir(const std::string& Dir) noexcept {
  p_dir = Dir;
}

const TreeDirItemPtr TreeDirItem::Parent() const noexcept {
  return p_parent.lock();
}

void TreeDirItem::setParent(const TreeDirItemPtr& Parent) noexcept {
  Parent->p_child_items.emplace_back(shared_from_this());
  p_parent = Parent;
}

size_t TreeDirItem::GetChildCount() const noexcept {
  return p_child_items.size();
}

TreeDirItemPtr TreeDirItem::GetChild(size_t index) const noexcept {
  if (index < 0 || index >= p_child_items.size()) return nullptr;

  return p_child_items.at(index);
}

size_t TreeDirItem::row() const noexcept {
  auto k_p = p_parent.lock();
  if (!k_p) return 0;
  auto k_it = std::find(k_p->p_child_items.begin(),
                        k_p->p_child_items.end(),
                        this->shared_from_this());

  auto k_dis = std::distance(p_parent.lock()->p_child_items.begin(), k_it);
  return k_dis;
}
}  // namespace doodle::motion::ui
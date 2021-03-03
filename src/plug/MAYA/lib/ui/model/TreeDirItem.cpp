#include <lib/ui/model/TreeDirItem.h>

#include <iostream>
namespace doodle::motion::ui {
TreeDirItem::TreeDirItem()
    : std::enable_shared_from_this<TreeDirItem>(),
      p_dir(),
      p_parent() {
}

TreeDirItem::TreeDirItem(std::string dir)
    : std::enable_shared_from_this<TreeDirItem>(),
      p_dir(std::move(dir)),
      p_parent() {
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

TreeDirItemPtr TreeDirItem::MakeChild(int position, std::string&& name) noexcept {
  auto child = std::make_shared<TreeDirItem>(std::move(name));
  this->p_child_items.insert(this->p_child_items.begin() + position,
                             child);
  child->p_parent = this->shared_from_this();

  return child;
}

bool TreeDirItem::removeChild(const TreeDirItemPtr point) {
  auto it = std::find(this->p_child_items.begin(), this->p_child_items.end(), point);
  if (it == p_child_items.end()) return false;
  this->p_child_items.erase(it);
  return true;
}

size_t TreeDirItem::ChildNumber() const noexcept {
  auto k_p = p_parent.lock();
  if (!k_p) return 0;
  auto k_it = std::find(k_p->p_child_items.begin(),
                        k_p->p_child_items.end(),
                        this->shared_from_this());

  auto k_dis = std::distance(p_parent.lock()->p_child_items.begin(), k_it);
  return k_dis;
}
}  // namespace doodle::motion::ui
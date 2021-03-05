#include <lib/ui/model/TreeDirItem.h>
#include <lib/kernel/MotionSetting.h>

#include <iostream>
#include <regex>

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

const FSys::path TreeDirItem::Dir() const noexcept {
  if (p_parent.expired()) {
    return kernel::MotionSetting::Get().MotionLibRoot() / p_dir;
  } else {
    return p_parent.lock()->Dir() / p_dir;
  }
}

void TreeDirItem::setDir(const FSys::path& Dir) noexcept {
  p_dir = Dir;
}

const TreeDirItemPtr TreeDirItem::Parent() const noexcept {
  return p_parent.lock();
}

void TreeDirItem::setParent(const TreeDirItemPtr& Parent) noexcept {
  Parent->p_child_items.emplace_back(shared_from_this());
  p_parent = Parent;
}

variant TreeDirItem::Data(int column) {
  auto require = variant{};
  if (column == 0) {
    require = p_dir.generic_string();
  }
  return require;
}

void TreeDirItem::setData(int column, const variant& Data) {
  if (Data.valueless_by_exception()) return;

  if (column == 0) {
    auto k_dir  = this->Dir();
    auto k_data = std::get<std::string>(Data);
    if (FSys::exists(k_dir) && !p_dir.empty()) {
      FSys::rename(k_dir, k_dir.parent_path() / k_data);
    } else {
      p_dir = k_data;
      this->makeDir();
    }
  }
}

size_t TreeDirItem::columnCount() const noexcept {
  return 1;
}

void TreeDirItem::removeColumn(int column) {
  std::runtime_error("无法移除");
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
  if (!name.empty()) {
    child->makeDir();
  }
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

void TreeDirItem::refreshChild() {
  auto p_dir = this->Dir();

  std::regex k_regex{p_dir.generic_string()};
  p_child_items.clear();

  for (auto it : FSys::directory_iterator(p_dir)) {
    if (it.is_directory()) {
      auto k_str        = std::regex_replace(it.path().generic_string(), k_regex, "");
      auto k_child      = std::make_shared<TreeDirItem>(k_str);
      k_child->p_parent = this->shared_from_this();
      p_child_items.emplace_back(std::move(k_child));
    }
  }
}

void TreeDirItem::makeDir() {
  FSys::create_directory(this->Dir());
}
}  // namespace doodle::motion::ui
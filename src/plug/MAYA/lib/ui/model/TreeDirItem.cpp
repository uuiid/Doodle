#include <lib/ui/model/TreeDirItem.h>
#include <lib/kernel/MotionSetting.h>

#include <iostream>
#include <regex>
#include <boost/locale.hpp>
#include <boost/numeric/conversion/cast.hpp>
namespace doodle::motion::ui {
TreeDirItem::TreeDirItem()
    : std::enable_shared_from_this<TreeDirItem>(),
      p_dir(),
      p_parent(),
      benignRemoveRows(),
      endRemoveRows(),
      benignInsertRows(),
      endInsertRows() {
}

TreeDirItem::TreeDirItem(FSys::path dir)
    : std::enable_shared_from_this<TreeDirItem>(),
      p_dir(std::move(dir)),
      p_parent(),
      benignRemoveRows(),
      endRemoveRows(),
      benignInsertRows(),
      endInsertRows() {
}

const FSys::path TreeDirItem::Dir(bool hasRoot) const noexcept {
  auto k_d = FSys::path{};
  if (hasRoot)
    k_d = kernel::MotionSetting::Get().MotionLibRoot();
  if (p_parent.expired()) {
    k_d = k_d / p_dir;
  } else {
    k_d = p_parent.lock()->Dir() / p_dir;
  }
  return k_d.lexically_normal();
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
    require = p_dir.generic_u8string();
  }
  return require;
}

void TreeDirItem::setData(int column, const variant& Data) {
  if (Data.valueless_by_exception()) return;

  if (column == 0) {
    auto k_dir  = this->Dir();
    auto k_data = std::get<std::string>(Data);
    auto k_wstr = boost::locale::conv::to_utf<wchar_t>(k_data, "UTF-8");
    if (FSys::exists(k_dir) && !p_dir.empty()) {
      FSys::rename(k_dir, k_dir.parent_path() / k_wstr);
      p_dir = k_wstr;
    } else {
      p_dir = k_wstr;
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

  benignInsertRows(position, 1);
  this->p_child_items.insert(this->p_child_items.begin() + position,
                             child);
  child->p_parent = this->shared_from_this();
  endInsertRows();

  return child;
}

bool TreeDirItem::removeChild(const TreeDirItemPtr point) {
  auto k_it = std::find(this->p_child_items.begin(), this->p_child_items.end(), point);
  if (k_it == p_child_items.end()) return false;

  benignRemoveRows(std::distance(this->p_child_items.begin(), k_it), 1);
  this->p_child_items.erase(k_it);
  endRemoveRows();

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
  auto k_dir = this->Dir();

  benignRemoveRows(0, boost::numeric_cast<int>(p_child_items.size()));
  p_child_items.clear();
  endRemoveRows();

  auto test = FSys::directory_iterator(k_dir);

  decltype(p_child_items) k_list{};

  //开始扫描目录
  for (auto& it : FSys::directory_iterator(k_dir)) {
    if (it.is_directory()) {
      auto k_str        = it.path().filename();
      auto k_child      = std::make_shared<TreeDirItem>(k_str);
      k_child->p_parent = this->shared_from_this();
      k_list.emplace_back(std::move(k_child));
    }
  }

  benignInsertRows(0, boost::numeric_cast<int>(k_list.size()));
  p_child_items = k_list;
  endInsertRows();
}

void TreeDirItem::recursiveRefreshChild() {
  this->refreshChild();
  for (auto&& item : p_child_items) {
    item->recursiveRefreshChild();
  }
}

void TreeDirItem::makeDir() {
  FSys::create_directory(this->Dir());
}
}  // namespace doodle::motion::ui
#pragma once

#include <MotionGlobal.h>
#include <QtCore/QVariant>
namespace doodle::motion::ui {
class TreeDirItem;
using TreeDirItemPtr = std::shared_ptr<TreeDirItem>;

class TreeDirItem : public std::enable_shared_from_this<TreeDirItem> {
 private:
  FSys::path p_dir;
  std::weak_ptr<TreeDirItem> p_parent;
  std::vector<TreeDirItemPtr> p_child_items;

 public:
  explicit TreeDirItem();
  explicit TreeDirItem(std::string dir);
  const FSys::path Dir() const noexcept;
  void setDir(const FSys::path& Dir) noexcept;

  const TreeDirItemPtr Parent() const noexcept;
  void setParent(const TreeDirItemPtr& Parent) noexcept;

  variant Data(int column);
  void setData(int column, const variant& Data);
  size_t columnCount() const noexcept;
  void removeColumn(int column);

  size_t GetChildCount() const noexcept;
  TreeDirItemPtr GetChild(size_t index) const noexcept;

  TreeDirItemPtr MakeChild(int position, std::string&& name) noexcept;
  bool removeChild(const TreeDirItemPtr point);

  size_t ChildNumber() const noexcept;

  void refreshChild();
  void makeDir();
  DOODLE_DISABLE_COPY(TreeDirItem);
};

}  // namespace doodle::motion::ui

Q_DECLARE_OPAQUE_POINTER(doodle::motion::ui::TreeDirItem)
// Q_DECLARE_METATYPE(doodle::motion::ui::TreeDirItem)
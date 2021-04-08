#pragma once

#include <lib/MotionGlobal.h>
#include <boost/signals2.hpp>

#include <QtWidgets/QListView>

namespace doodle::motion::ui {
class MotionView : public QListView {
  Q_OBJECT

  TreeDirItemPtr p_TreeDirItem;

 public:
  MotionView(QWidget* parent = nullptr);

  void setTreeNode(const decltype(p_TreeDirItem)& item);

  boost::signals2::signal<void(const kernel::MotionFilePtr& data)> sig_chickItem;

 protected:
  //上下文菜单
  void contextMenuEvent(QContextMenuEvent* event) override;

  // //拖拽函数
  // void dragMoveEvent(QDragMoveEvent* event) override;
  // //拖拽函数
  // void dragLeaveEvent(QDragLeaveEvent* event) override;
  // //拖拽函数
  // void dragEnterEvent(QDragEnterEvent* event) override;
  // //拖拽函数
  // void dropEvent(QDropEvent* event) override;

 private:
  void createFbxAction(const FSys::path& path);
  void updateIcon();
  void updateVideo();
  void importFbxAction();

  void doodleChicked(const QModelIndex& index);
};

}  // namespace doodle::motion::ui
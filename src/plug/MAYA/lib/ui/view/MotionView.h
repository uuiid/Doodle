#pragma once

#include <MotionGlobal.h>
#include <QtWidgets/QListView>

namespace doodle::motion::ui {
class MotionView : public QListView {
 private:
  Q_OBJECT
 public:
  MotionView(QWidget* parent = nullptr);

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
};

}  // namespace doodle::motion::ui
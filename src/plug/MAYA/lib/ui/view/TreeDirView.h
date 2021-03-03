#pragma once

#include <MotionGlobal.h>
#include <QtWidgets/QTreeView>
namespace doodle::motion::ui {
class TreeDirView : public QTreeView {
  Q_OBJECT
 private:
 public:
  TreeDirView(QWidget* parent = nullptr);
  ~TreeDirView();

 protected:
  //上下文菜单
  void contextMenuEvent(QContextMenuEvent* event) override;

 private:
  void createDir();
  void editDir();
};

}  // namespace doodle::motion::ui
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
};

}  // namespace doodle::motion::ui
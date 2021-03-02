#include <lib/ui/MotionLibWidget.h>

#include <lib/ui/model/TreeDirModel.h>
#include <lib/ui/model/MotionModel.h>
#include <lib/ui/view/MotionView.h>
#include <lib/ui/view/TreeDirView.h>

#include <QtWidgets/qgridlayout.h>

namespace doodle::motion::ui {
MotionLibWidget::MotionLibWidget(QWidget *parent)
    : QWidget(parent) {
  auto layout         = new QGridLayout(this);
  auto k_tree_model   = new TreeDirModel(this);
  auto k_motion_model = new MotionModel(this);

  auto k_tree_view   = new QTreeView();
  auto k_motion_view = new MotionView();

  k_tree_view->setModel(k_tree_model);
  k_motion_view->setModel(k_motion_model);

  layout->addWidget(k_tree_view, 0, 0, 1, 1);
  layout->addWidget(k_motion_view, 0, 1, 1, 1);
  layout->setColumnStretch(0, 1);
  layout->setColumnStretch(1, 5);
}
}  // namespace doodle::motion::ui
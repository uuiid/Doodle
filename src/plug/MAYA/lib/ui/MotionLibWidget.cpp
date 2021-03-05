#include <lib/ui/MotionLibWidget.h>

#include <lib/ui/model/TreeDirItem.h>

#include <lib/ui/model/TreeDirModel.h>
#include <lib/ui/model/MotionModel.h>
#include <lib/ui/view/MotionView.h>
#include <lib/ui/view/TreeDirView.h>

#include <QtWidgets/qgridlayout.h>
#include <QtWidgets/qfilesystemmodel.h>
namespace doodle::motion::ui {
MotionLibWidget::MotionLibWidget(QWidget *parent)
    : QWidget(parent) {
  auto layout         = new QGridLayout(this);
  auto k_tree_model   = new TreeDirModel(this);
  auto k_motion_model = new MotionModel(this);

  auto k_tree_view   = new TreeDirView();
  auto k_motion_view = new MotionView();

  k_tree_view->setModel(k_tree_model);
  k_motion_view->setModel(k_motion_model);

  layout->addWidget(k_tree_view, 0, 0, 1, 1);
  layout->addWidget(k_motion_view, 0, 1, 1, 1);
  layout->setColumnStretch(0, 1);
  layout->setColumnStretch(1, 5);

  k_tree_view->sig_chickItem.connect(
      [k_motion_model, k_motion_view](const FSys::path &path, const QModelIndex &index) {
        auto k_lists = kernel::MotionFile::getAll(path);
        k_motion_model->setLists(k_lists);
        auto k_data = static_cast<TreeDirItem *>(index.internalPointer())->shared_from_this();
        k_motion_view->setTreeNode(k_data);
      });
}
}  // namespace doodle::motion::ui
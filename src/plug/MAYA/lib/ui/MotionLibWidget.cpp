#include <lib/ui/MotionLibWidget.h>

#include <lib/ui/model/TreeDirItem.h>

#include <lib/ui/model/TreeDirModel.h>
#include <lib/ui/model/MotionModel.h>
#include <lib/ui/view/MotionView.h>
#include <lib/ui/view/TreeDirView.h>
#include <lib/ui/view/MotionAttrbuteView.h>

#include <QtWidgets/qgridlayout.h>
#include <QtWidgets/qfilesystemmodel.h>
#include <QtCore/qsortfilterproxymodel.h>

namespace doodle::motion::ui {
MotionLibWidget::MotionLibWidget(QWidget *parent)
    : QWidget(parent) {
  auto layout         = new QGridLayout(this);
  auto k_tree_model   = new TreeDirModel(this);
  auto k_motion_model = new MotionModel(this);
  auto k_motion_sort  = new MotionModelSortFilter(this);

  auto k_tree_view   = new TreeDirView();
  auto k_motion_view = new MotionView();
  auto k_attr_vire   = new MotionAttrbuteView();

  k_motion_sort->sort(0);

  k_tree_view->setModel(k_tree_model);
  k_motion_sort->setSourceModel(k_motion_model);
  k_motion_view->setModel(k_motion_sort);

  layout->addWidget(k_tree_view, 0, 0, 1, 1);
  layout->addWidget(k_motion_view, 0, 1, 1, 1);
  layout->addWidget(k_attr_vire, 0, 2, 1, 1);
  layout->setColumnStretch(0, 1);
  layout->setColumnStretch(1, 5);
  layout->setColumnStretch(2, 3);

  k_tree_view->sig_chickItem.connect(
      [k_motion_model, k_motion_view, k_attr_vire, k_motion_sort](const FSys::path &path, const QModelIndex &index) {
        //设置显示模型和排序
        auto k_lists = kernel::MotionFile::getAll(path);
        k_motion_model->setLists(k_lists);
        k_motion_sort->sort(0);
        //设置视图持有的根项目
        auto k_data = static_cast<TreeDirItem *>(index.internalPointer())->shared_from_this();
        k_motion_view->setTreeNode(k_data);
        //清除详细视图显示
        k_attr_vire->doodleClear();
      });

  k_motion_view->sig_chickItem.connect(
      [k_attr_vire](const kernel::MotionFilePtr &data) {
        k_attr_vire->setMotionFile(data);
      });
}
}  // namespace doodle::motion::ui
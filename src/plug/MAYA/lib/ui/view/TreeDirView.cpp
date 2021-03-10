#include <lib/ui/view/TreeDirView.h>

#include <lib/ui/model/TreeDirModel.h>

#include <QtGui/QContextMenuEvent>

#include <QtWidgets/qmenu.h>
#include <QtWidgets/qinputdialog.h>
#include <QtWidgets/qmessagebox.h>

namespace doodle::motion::ui {
TreeDirView::TreeDirView(QWidget* parent)
    : QTreeView(parent),
      sig_chickItem() {
  setUniformRowHeights(true);
  connect(this, &TreeDirView::clicked,
          this, &TreeDirView::doodleChicked);
}

TreeDirView::~TreeDirView() {
}

void TreeDirView::contextMenuEvent(QContextMenuEvent* event) {
  auto menu       = new QMenu(this);
  auto add_active = menu->addAction(tr("创建目录"));
  connect(add_active, &QAction::triggered,
          this, &TreeDirView::createDir);
  if (this->selectionModel()->hasSelection()) {
    auto edit_active = menu->addAction(tr("编辑项目"));
    connect(edit_active, &QAction::triggered,
            this, &TreeDirView::editDir);

    auto refresh_active = menu->addAction(tr("刷新子项"));
    connect(refresh_active, &QAction::triggered,
            this, [=]() {
              auto k_model = dynamic_cast<TreeDirModel*>(this->model());
              k_model->refreshChild(this->selectionModel()->currentIndex());
            });
  }

  menu->move(event->globalPos());
  menu->show();
}

void TreeDirView::createDir() {
  auto k_model = dynamic_cast<TreeDirModel*>(this->model());

  auto k_index = QModelIndex();
  // if (this->selectionModel()->hasSelection()) {
  //   k_index = this->selectionModel()->currentIndex();
  // }

  auto k_str = QInputDialog::getText(this, tr("创建类别"), tr("类别"));
  //条件创建
  if (k_str.isEmpty()) return;
  if (k_str == "etc") QMessageBox::warning(this, tr("失败"), tr("名称不可为 etc"));

  auto k_insert_bool = k_model->insertRow(0, k_index);
  if (k_insert_bool)
    k_model->setData(k_model->index(0, 0, k_index), k_str, Qt::EditRole);
  else
    QMessageBox::warning(this, tr("插入失败"), tr("插入失败,请联系制作人员"));
}

void TreeDirView::editDir() {
  if (!this->selectionModel()->hasSelection()) return;
  auto k_index = this->selectionModel()->currentIndex();
  edit(k_index);
}

void TreeDirView::doodleChicked(const QModelIndex& index) {
  auto l_data = static_cast<TreeDirItem*>(index.internalPointer())->shared_from_this();
  if (!l_data) return;
  sig_chickItem(l_data->Dir(), index);
}
}  // namespace doodle::motion::ui
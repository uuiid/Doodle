#include <lib/ui/view/TreeDirView.h>

#include <lib/ui/model/TreeDirModel.h>

#include <QtWidgets/qmenu.h>
#include <QtGui/QContextMenuEvent>
#include <QtWidgets/qinputdialog.h>
#include <QtWidgets/qmessagebox.h>

namespace doodle::motion::ui {
TreeDirView::TreeDirView(QWidget* parent)
    : QTreeView(parent) {
  setUniformRowHeights(true);
}

TreeDirView::~TreeDirView() {
}

void TreeDirView::contextMenuEvent(QContextMenuEvent* event) {
  auto menu       = new QMenu(this);
  auto add_active = menu->addAction("创建目录");
  connect(add_active, &QAction::triggered,
          this, &TreeDirView::createDir);
  if (this->selectionModel()->hasSelection()) {
    auto edit_active = menu->addAction("编辑项目");
    connect(edit_active, &QAction::triggered,
            this, &TreeDirView::editDir);
  }

  menu->move(event->globalPos());
  menu->show();
}

void TreeDirView::createDir() {
  auto k_model = dynamic_cast<TreeDirModel*>(this->model());

  auto k_index = QModelIndex();
  if (this->selectionModel()->hasSelection()) {
    k_index = this->selectionModel()->currentIndex();
  }

  auto k_str = QInputDialog::getText(this, tr("创建类别"), tr("类别"));
  if (k_str.isEmpty()) return;
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
}  // namespace doodle::motion::ui
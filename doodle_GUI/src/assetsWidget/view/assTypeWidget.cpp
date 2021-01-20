//
// Created by teXiao on 2020/10/16.
//

#include "assTypeWidget.h"

#include "src/assetsWidget/model/assTypeModel.h"
#include < core_Cpp.h>
#include <QMenu>

#include <QContextMenuEvent>
#include <QtWidgets/QMessageBox>

DOODLE_NAMESPACE_S
fileTypeAssDelegate::fileTypeAssDelegate(QObject *parent) : QStyledItemDelegate(parent) {
}
QWidget *fileTypeAssDelegate::createEditor(QWidget *parent,
                                           const QStyleOptionViewItem &option,
                                           const QModelIndex &index) const {
  auto *fileType = new QComboBox(parent);

  auto assClass = coreDataManager::get().getAssClassPtr();
  QStringList list;
  list << "sourceimages"
       << "scenes"
       << QString("%1_UE4").arg(assClass->getAssClassQ(true))
       << "rig"
       << QString("%1_low").arg(assClass->getAssClassQ(true));

  fileType->addItems(list);

  return fileType;
}
void fileTypeAssDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const {
  auto *edit = dynamic_cast<QComboBox *>(editor);

  edit->setCurrentIndex(0);
}
void fileTypeAssDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const {
  auto *edit = dynamic_cast<QComboBox *>(editor);

  auto value                      = edit->currentText();
  QMessageBox::StandardButton box = QMessageBox::information(dynamic_cast<QWidget *>(this->parent()),
                                                             tr("警告:"),
                                                             tr("将 %1 类型提交到服务器").arg(value),
                                                             QMessageBox::Yes | QMessageBox::Cancel);
  if (box == QMessageBox::Yes)
    model->setData(index, value, Qt::EditRole);
  else
    model->removeRow(index.row(), index);
}
void fileTypeAssDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                                               const QModelIndex &index) const {
  editor->setGeometry(option.rect);
}

assTypeWidget::assTypeWidget(QWidget *parent)
    : QListView(parent),
      p_menu_(nullptr),
      p_model_(nullptr) {
  setItemDelegate(new fileTypeAssDelegate(this));
  connect(this, &assTypeWidget::clicked,
          this, &assTypeWidget::_doodle_type_emit);
}
void assTypeWidget::setModel(QAbstractItemModel *model) {
  auto k_model = dynamic_cast<assTypeModel *>(model);
  if (k_model)
    p_model_ = k_model;
  QAbstractItemView::setModel(model);
}

void assTypeWidget::inserttype() {
  int raw = selectionModel()->currentIndex().row();
  p_model_->insertRow(raw);
  setCurrentIndex(p_model_->index(raw));
  edit(p_model_->index(raw));
}
void assTypeWidget::_doodle_type_emit(const QModelIndex &index) {
  auto k_asstype = index.data(Qt::UserRole).value<assType *>();
  if (k_asstype) {
    coreDataManager::get().setAssTypePtr(k_asstype->shared_from_this());
    chickItem(k_asstype->shared_from_this(), filterState::useFilter);
  }
}
void assTypeWidget::mousePressEvent(QMouseEvent *event) {
  QListView::mousePressEvent(event);
  if (!indexAt(event->pos()).isValid()) {
    selectionModel()->clearSelection();
    clearSelection();
    update();
    // p_model_->reInit();
    coreDataManager::get().setAssTypePtr(nullptr);
    chickItem(nullptr, filterState::useFilter);
  }
}
//void assTypeWidget::contextMenuEvent(QContextMenuEvent *event) {
//  if(!p_menu_){
//    p_menu_ = new QMenu(this);
//    auto add_ass_type = new QAction(p_menu_);
//    add_ass_type->setText(tr("添加"));
//    connect(add_ass_type,&QAction::triggered,
//            this,&assTypeWidget::inserttype);
//    p_menu_->addAction(add_ass_type);
//  }
//  p_menu_->move(event->globalPos());
//  p_menu_->show();
//}

DOODLE_NAMESPACE_E

//
// Created by teXiao on 2020/10/16.
//

#include "assClassWidget.h"
#include <core_doQt.h>
#include <QtGui/qevent.h>
#include <QtWidgets/qapplication.h>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMenu>
#include <src/assClassModel.h>


DOODLE_NAMESPACE_S
assClassDelegate::assClassDelegate(QObject* parent)
  : QStyledItemDelegate(parent) {}

QWidget* assClassDelegate::createEditor(QWidget* parent,
  const QStyleOptionViewItem& option,
  const QModelIndex& index) const {
  auto lineEdit = new QLineEdit(parent);
  return lineEdit;
}
void doodle::assClassDelegate::setEditorData(QWidget* editor,
  const QModelIndex& index) const {
  auto lineEdit = dynamic_cast<QLineEdit*>(editor);
  lineEdit->setText("");
}

void assClassDelegate::setModelData(QWidget* editor, QAbstractItemModel* model,
  const QModelIndex& index) const {
  auto lineEdit = dynamic_cast<QLineEdit*>(editor);
  model->setData(index, lineEdit->text());
}

void assClassDelegate::updateEditorGeometry(QWidget* editor,
  const QStyleOptionViewItem& option,
  const QModelIndex& index) const {
  editor->setGeometry(option.rect);
}


assClassWidget::assClassWidget(QWidget* parent)
  : QListView(parent), p_model_(), p_menu_(nullptr) {
  setStatusTip("右键添加文件");
  connect(this, &assClassWidget::clicked, this,
    &assClassWidget::_doodle_ass_emit);

  setEditTriggers(QAbstractItemView::NoEditTriggers);
  setItemDelegate(new assClassDelegate(this));
}
void assClassWidget::setModel(QAbstractItemModel* model) {
  auto t_model_ = dynamic_cast<assClassModel*>(model);
  if (t_model_) p_model_ = t_model_;
  QAbstractItemView::setModel(model);
}
void assClassWidget::insertAss() {
  int raw = selectionModel()->currentIndex().row();
  p_model_->insertRow(raw);
  setCurrentIndex(p_model_->index(raw));  //设置当前索引
  edit(p_model_->index(raw));             //编译当前添加
}
void assClassWidget::editAssName() {
  if (selectionModel()->hasSelection()) edit(selectionModel()->currentIndex());
}
void assClassWidget::_doodle_ass_emit(const QModelIndex& index) {
  auto assClass =
    p_model_->data(index, Qt::UserRole).value<doCore::assClassPtr>();
  doCore::coreDataManager::get().setAssClassPtr(assClass);
  emit initEmited();
}
void assClassWidget::contextMenuEvent(QContextMenuEvent* event) {
  if (p_menu_) {
    p_menu_->clear();
  }
  else {
    p_menu_ = new QMenu(this);
  }
  auto* add_ass = new QAction(p_menu_);
  connect(add_ass, &QAction::triggered, this, &assClassWidget::insertAss);
  add_ass->setText(tr("添加资产"));
  add_ass->setStatusTip(tr("添加资产"));
  p_menu_->addAction(add_ass);

  if (selectionModel()->hasSelection()) {
    auto* edit_name = new QAction(p_menu_);
    connect(edit_name, &QAction::triggered, this, &assClassWidget::editAssName);
    edit_name->setText(tr("修改中文名称"));
    connect(edit_name, &QAction::triggered, this, [=]() {
      this->edit(selectionModel()->currentIndex());
      });
    p_menu_->addAction(edit_name);
  }
  p_menu_->move(event->globalPos());
  p_menu_->show();
}

DOODLE_NAMESPACE_E
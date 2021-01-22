/*
 * @Author: your name
 * @Date: 2020-10-19 13:26:31
 * @LastEditTime: 2020-12-14 17:49:32
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_GUI\src\assClassWidget.cpp
 */
//
// Created by teXiao on 2020/10/16.
//

#include "assClassWidget.h"
#include < core_Cpp.h>
#include <src/assetsWidget/model/assClassModel.h>
#include <QtGui/qevent.h>
#include <QtWidgets/qapplication.h>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>

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
  lineEdit->setText(index.data(Qt::EditRole).toString());
}

void assClassDelegate::setModelData(QWidget* editor, QAbstractItemModel* model,
                                    const QModelIndex& index) const {
  auto lineEdit = dynamic_cast<QLineEdit*>(editor);
  if (!lineEdit->text().isEmpty() && !lineEdit->text().isNull()) {
    model->setData(index, lineEdit->text());
  }
}

void assClassDelegate::updateEditorGeometry(QWidget* editor,
                                            const QStyleOptionViewItem& option,
                                            const QModelIndex& index) const {
  editor->setGeometry(option.rect);
}

assClassWidget::assClassWidget(QWidget* parent)
    : QListView(parent), p_menu_(nullptr) {
  setStatusTip("右键添加文件");
  connect(this, &assClassWidget::clicked, this,
          &assClassWidget::doodle_clicked_emit);

  setEditTriggers(QAbstractItemView::NoEditTriggers);
  setItemDelegate(new assClassDelegate(this));
}
void assClassWidget::setModel(QAbstractItemModel* model) {
  // auto t_model_ = dynamic_cast<assClassModel*>(model);
  // if (t_model_) p_model_ = t_model_;
  QAbstractItemView::setModel(model);
}
void assClassWidget::insertAss() {
  int raw = selectionModel()->currentIndex().row() + 1;
  model()->insertRow(raw);
  setCurrentIndex(model()->index(raw, 0));  //设置当前索引
  edit(model()->index(raw, 0));             //编译当前添加
}
void assClassWidget::editAssName() {
  if (selectionModel()->hasSelection()) edit(selectionModel()->currentIndex());
}

void assClassWidget::deleteSQLFile() {
  for (auto&& i : assFileSqlInfo::Instances()) {
    if (i->getAssClass() == coreDataManager::get().getAssClassPtr()) {
      QMessageBox::warning(this, tr("注意"), tr("这个条目中还有内容,无法删除"));
      return;
    }
  }
  model()->removeRow(selectionModel()->currentIndex().row());
}

void assClassWidget::doodle_clicked_emit(const QModelIndex& index) {
  auto k_assClass = model()->data(index, Qt::UserRole).value<assClass*>();
  if (k_assClass)
    chickItem(k_assClass->shared_from_this());
}
void assClassWidget::contextMenuEvent(QContextMenuEvent* event) {
  if (p_menu_) {
    p_menu_->clear();
  } else {
    p_menu_ = new QMenu(this);
  }
  if (coreDataManager::get().getAssDepPtr()) {
    auto* add_ass = new QAction(p_menu_);
    connect(add_ass, &QAction::triggered, this, &assClassWidget::insertAss);
    add_ass->setText(tr("添加资产"));
    add_ass->setStatusTip(tr("添加资产"));
    p_menu_->addAction(add_ass);

    if (selectionModel()->hasSelection()) {
      //修改中文名称
      auto edit_name = new QAction(p_menu_);
      edit_name->setText(tr("修改中文名称"));
      connect(edit_name, &QAction::triggered, this,
              [=]() { this->edit(selectionModel()->currentIndex()); });
      p_menu_->addAction(edit_name);

      //删除
      auto delete_arction = new QAction();
      delete_arction->setText("删除条目");
      connect(delete_arction, &QAction::triggered, this,
              &assClassWidget::deleteSQLFile);
      p_menu_->addAction(delete_arction);
    }
    p_menu_->move(event->globalPos());
    p_menu_->show();
  }
}

DOODLE_NAMESPACE_E
//
// Created by teXiao on 2020/10/16.
//

#include "fileTypeAssWidget.h"

#include "fileTypeAssModel.h"
#include "src/filetype.h"
#include <QModelIndex>
#include <QMenu>

#include <QContextMenuEvent>

DOODLE_NAMESPACE_S
fileTypeAssWidget::fileTypeAssWidget(QWidget *parent)
    : QListView(parent),
      p_type_ptr_list_(),
      p_menu_(nullptr),
      p_model_(nullptr),
      p_ass_type_ptr_(nullptr) {
  connect(this,&fileTypeAssWidget::clicked,
          this,&fileTypeAssWidget::_doodle_type_emit);
}
void fileTypeAssWidget::setModel(QAbstractItemModel *model) {
  auto k_model = dynamic_cast<fileTypeAssModel *>(model);
  if (k_model)
    p_model_ = k_model;
  QAbstractItemView::setModel(model);
}
void fileTypeAssWidget::init(const doCore::assTypePtr &ass_type_ptr) {
  p_ass_type_ptr_ = ass_type_ptr;
  p_model_->init(ass_type_ptr);
}

void fileTypeAssWidget::clear() {
  p_ass_type_ptr_.reset();
  p_model_->clear();
}
void fileTypeAssWidget::inserttype() {
 int raw = selectionModel()->currentIndex().row();
 p_model_->insertRow(raw);
 setCurrentIndex(p_model_->index(raw));
 edit(p_model_->index(raw));
}
void fileTypeAssWidget::_doodle_type_emit(const QModelIndex &index) {
  emit filetypeEmited(index.data(Qt::UserRole).value<doCore::fileTypePtr>());
}
void fileTypeAssWidget::contextMenuEvent(QContextMenuEvent *event) {
  if(p_ass_type_ptr_ && !p_menu_){
    p_menu_ = new QMenu(this);
    auto add_ass_type = new QAction(p_menu_);
    add_ass_type->setText(tr("添加"));
    connect(add_ass_type,&QAction::triggered,
            this,&fileTypeAssWidget::inserttype);
    p_menu_->addAction(add_ass_type);
  }
  p_menu_->move(event->globalPos());
  p_menu_->show();
}

DOODLE_NAMESPACE_E
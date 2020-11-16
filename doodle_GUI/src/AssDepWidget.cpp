//
// Created by teXiao on 2020/10/15.
//

#include "AssDepWidget.h"
#include "AssDepModel.h"
#include <core_doQt.h>
DOODLE_NAMESPACE_S
AssDepWidget::AssDepWidget(QWidget *parent) : QListView(parent) {
  setFlow(QListView::LeftToRight);
  connect(this, &AssDepWidget::clicked,
          this, &AssDepWidget::_doodle_emit);
}
void AssDepWidget::setModel(QAbstractItemModel *model) {
  auto model_ = dynamic_cast<AssDepModel *>(model);
  if (model_)
    p_file_class_ass_model_ = model_;
  QAbstractItemView::setModel(model);
}
void AssDepWidget::init() {
  if (p_file_class_ass_model_)
    p_file_class_ass_model_->init();
}
void AssDepWidget::_doodle_emit(const QModelIndex &index) {
  emit fileClassEmit(p_file_class_ass_model_
                         ->data(index, Qt::UserRole).value<doCore::assDepPtr>());
}
AssDepWidget::~AssDepWidget() = default;
DOODLE_NAMESPACE_E

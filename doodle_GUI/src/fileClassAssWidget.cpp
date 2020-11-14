//
// Created by teXiao on 2020/10/15.
//

#include "fileClassAssWidget.h"
#include "AssDepModel.h"
#include "src/shotClass.h"
DOODLE_NAMESPACE_S
fileClassAssWidget::fileClassAssWidget(QWidget *parent) : QListView(parent) {
  setFlow(QListView::LeftToRight);
  connect(this, &fileClassAssWidget::clicked,
          this, &fileClassAssWidget::_doodle_emit);
}
void fileClassAssWidget::setModel(QAbstractItemModel *model) {
  auto model_ = dynamic_cast<AssDepModel *>(model);
  if (model_)
    p_file_class_ass_model_ = model_;
  QAbstractItemView::setModel(model);
}
void fileClassAssWidget::init() {
  if (p_file_class_ass_model_)
    p_file_class_ass_model_->init();
}
void fileClassAssWidget::_doodle_emit(const QModelIndex &index) {
  emit fileClassEmit(p_file_class_ass_model_
                         ->data(index, Qt::UserRole).value<doCore::shotClassPtr>());
}
fileClassAssWidget::~fileClassAssWidget() = default;
DOODLE_NAMESPACE_E

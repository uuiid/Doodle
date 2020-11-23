//
// Created by teXiao on 2020/10/15.
//

#include <core_doQt.h>
#include <src/assDepModel.h>
#include <src/AssDepWidget.h>

DOODLE_NAMESPACE_S
AssDepWidget::AssDepWidget(QWidget *parent)
    : QListView(parent), p_file_class_ass_model_(nullptr) {
  setFlow(QListView::LeftToRight);
  connect(this, &AssDepWidget::clicked, this, &AssDepWidget::_doodle_emit);
}
void AssDepWidget::setModel(QAbstractItemModel *model) {
  auto model_ = dynamic_cast<assDepModel *>(model);
  if (model_) p_file_class_ass_model_ = model_;
  QAbstractItemView::setModel(model);
}
void AssDepWidget::_doodle_emit(const QModelIndex &index) {
  auto assdep = p_file_class_ass_model_->data(index, Qt::UserRole)
                    .value<doCore::assDepPtr>();
  doCore::coreDataManager::get().setAssDepPtr(assdep);
  emit initEmit();
}
AssDepWidget::~AssDepWidget() = default;
DOODLE_NAMESPACE_E

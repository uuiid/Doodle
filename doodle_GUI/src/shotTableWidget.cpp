//
// Created by teXiao on 2020/10/13.
//

#include <QJsonArray>
#include <QMenu>
#include <QContextMenuEvent>
#include <QHeaderView>

#include "src/filetype.h"
#include "src/shotfilesqlinfo.h"
#include "Logger.h"
#include "shotTableModel.h"
#include "shotTableWidget.h"


DOODLE_NAMESPACE_S
//-----------------------------------自定义shot小部件---------------------------------------------//
shotTableWidget::shotTableWidget(QWidget *parent) : QTableView(parent), p_model_(nullptr) {
  setSelectionBehavior(QAbstractItemView::SelectRows);//行选
  setSelectionMode(QAbstractItemView::SingleSelection);//单选
}
void shotTableWidget::init(const doCore::fileTypePtr &file_type_ptr) {
  p_type_ptr_ = file_type_ptr;
  p_model_->init(file_type_ptr);
  horizontalHeader()->setVisible(true);
  horizontalHeader()->setMinimumHeight(20);
  horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
}
void shotTableWidget::insertShot() {
  DOODLE_LOG_INFO << "提交文件";
}
void shotTableWidget::contextMenuEvent(QContextMenuEvent *event) {
  p_menu_ = new QMenu(this);
  if (p_type_ptr_) {
    auto *action = new QAction(p_menu_);

    connect(action, &QAction::triggered,
            this, &shotTableWidget::insertShot);
    action->setText(tr("提交文件"));
    action->setStatusTip(tr("提交所需要的文件"));
    p_menu_->addAction(action);
  }
  p_menu_->move(event->globalPos());
  p_menu_->show();
}
void shotTableWidget::clear() {
  p_type_ptr_ = nullptr;
  p_model_->clear();
}
void shotTableWidget::setModel(QAbstractItemModel *model) {
  auto p_model = dynamic_cast<shotTableModel *>(model);
  if(p_model)
    p_model_ = p_model;
  QTableView::setModel(model);
}

DOODLE_NAMESPACE_E


//
// Created by teXiao on 2020/10/17.
//

#include "assTableWidght.h"

#include "assTableModel.h"
#include "Logger.h"
#include <core_doQt.h>
#include <QMenu>
#include <QContextMenuEvent>

#include <QHeaderView>
#include <QStyledItemDelegate>

DOODLE_NAMESPACE_S
assTableWidght::assTableWidght(QWidget *parent)
    : QTableView(parent),
      p_info_ptr_list_(),
      p_menu_(nullptr),
      p_model_(nullptr),
      p_file_type_ptr_(nullptr) {
  setSelectionBehavior(QAbstractItemView::SelectRows);//行选
  setSelectionMode(QAbstractItemView::SingleSelection);//单选
  setSelectionBehavior(QAbstractItemView::SelectRows);
  setShowGrid(false);
  setFrameShape(QFrame::NoFrame);
}
void assTableWidght::setModel(QAbstractItemModel *model) {
  auto k_model_ = dynamic_cast<assTableModel *>(model);
  if (k_model_)
    p_model_ = k_model_;
  QTableView::setModel(model);
}
void assTableWidght::init(const doCore::assTypePtr &file_type_ptr) {
  p_file_type_ptr_ = file_type_ptr;
  p_model_->init(file_type_ptr);
  horizontalHeader()->setVisible(true);
  horizontalHeader()->setMinimumHeight(20);
  horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
  horizontalHeader()->setHighlightSections(false);


}
void assTableWidght::clear() {
  p_file_type_ptr_.reset();
  p_model_->clear();
}
void assTableWidght::insertAss() {
  DOODLE_LOG_INFO << "插入ass数据";
}
void assTableWidght::contextMenuEvent(QContextMenuEvent *event) {
  //只有在菜单指针为空 并且 持有上一级的的情况下才进行显示菜单
  if (!p_menu_ && p_file_type_ptr_) {
    p_menu_ = new QMenu(this);
    auto add_ass = new QAction(p_menu_);
    add_ass->setText(tr("提交文件"));
    p_menu_->addAction(add_ass);
  }
  if (p_file_type_ptr_) {
    p_menu_->move(event->globalPos());
    p_menu_->show();
  }
}

DOODLE_NAMESPACE_E
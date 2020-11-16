//
// Created by teXiao on 2020/10/17.
//
#include <src/assTableWidght.h>
#include <boost/algorithm/string.hpp>
#include <core_doQt.h>
#include <Logger.h>
#include <QContextMenuEvent>
#include <QHeaderView>
#include <QMenu>
#include <QStyledItemDelegate>
#include <QtWidgets/qmessagebox.h>
#include <src/assTableModel.h>
#include <src/Toolkit.h>
#include <string>
DOODLE_NAMESPACE_S
assTableWidght::assTableWidght(QWidget *parent)
    : QTableView(parent),
      p_info_ptr_list_(),
      p_menu_(nullptr),
      p_model_(nullptr),
      p_file_type_ptr_(nullptr) {
  setSelectionBehavior(QAbstractItemView::SelectRows);   //行选
  setSelectionMode(QAbstractItemView::SingleSelection);  //单选
  setSelectionBehavior(QAbstractItemView::SelectRows);
  setShowGrid(false);
  setFrameShape(QFrame::NoFrame);
}
void assTableWidght::setModel(QAbstractItemModel *model) {
  auto k_model_ = dynamic_cast<assTableModel *>(model);
  if (k_model_) p_model_ = k_model_;
  QTableView::setModel(model);
  init();
}
void assTableWidght::init() {
  horizontalHeader()->setVisible(true);
  horizontalHeader()->setMinimumHeight(20);
  horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
  horizontalHeader()->setHighlightSections(false);
}
void assTableWidght::insertAss() { DOODLE_LOG_INFO << "插入ass数据"; }
void assTableWidght::contextMenuEvent(QContextMenuEvent *event) {
  //只有在菜单指针为空 并且 持有上一级的的情况下才进行显示菜单
  if (p_menu_) {
    p_menu_->clear();
  } else {
    p_menu_ = new QMenu(this);
  }
  auto sub_file = new QAction(p_menu_);
  sub_file->setText(tr("提交文件"));
  p_menu_->addAction(sub_file);

  auto index = p_model_->index(selectionModel()->currentIndex().row(),
                               4);  //获得模型索引

  if (selectionModel()->hasSelection()) {
    auto k_openFile = new QAction();
    k_openFile->setText("打开文件所在位置");
    connect(k_openFile, &QAction::triggered, this, [=] {
      toolkit::openPath(index.data(Qt::UserRole).value<doCore::assInfoPtr>(), true);
    });
    p_menu_->addAction(k_openFile);
  }

  p_menu_->move(event->globalPos());
  p_menu_->show();
}

DOODLE_NAMESPACE_E
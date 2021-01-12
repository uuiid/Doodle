/*
 * @Author: your name
 * @Date: 2020-10-10 10:26:01
 * @LastEditTime: 2020-10-10 15:58:55
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit  
 * @FilePath: \Doodle\doodle_GUI\src\shotClassWidget.cpp
 */
#include "Logger.h"
#include "src/shotsWidght/model/shotClassModel.h"
#include "shotClassWidget.h"
#include < core_Cpp.h>

#include <QMenu>
#include <QContextMenuEvent>

DOODLE_NAMESPACE_S

/* --------------------------------- 自定义小部件 --------------------------------- */
shotClassWidget::shotClassWidget(QWidget *parent)
    : QListView(parent),
      p_model_(nullptr),
      p_fileClass_menu(nullptr) {
  setStatusTip(tr("使用右键直接添加部门类型"));

  connect(this, &shotClassWidget::clicked,
          this, &shotClassWidget::_doodle_fileclass_emit);
}

void shotClassWidget::insertFileClass() {
  int kRow = selectionModel()->currentIndex().row() + 1;
  p_model_->insertRow(kRow, QModelIndex());

  setCurrentIndex(p_model_->index(kRow));
  edit(p_model_->index(kRow));
}

void shotClassWidget::_doodle_fileclass_emit(const QModelIndex &index) {
  coreDataManager::get().setShotClassPtr(
      index.data(Qt::UserRole).value<shotClass *>()->shared_from_this());
  doodleUseFilter(filterState::useFilter);
}

void shotClassWidget::mousePressEvent(QMouseEvent *event) {
  QListView::mousePressEvent(event);
  if (!indexAt(event->pos()).isValid()) {
    clear();
  }
}

//void shotClassWidget::contextMenuEvent(QContextMenuEvent *event) {
//  p_fileClass_menu = new QMenu(this);
//
//  if (p_shot) {
//    auto *action = new QAction(this);
//
//    connect(action, &QAction::triggered, this,
//            &shotClassWidget::insertFileClass);
//    action->setText(tr("添加部门"));
//    action->setToolTip(tr("添加本部门"));
//    p_fileClass_menu->addAction(action);
//  }
//  p_fileClass_menu->move(event->globalPos());
//  p_fileClass_menu->show();
//  DOODLE_LOG_INFO ( "显示部门上下文菜单");
//}

void shotClassWidget::clear() {
  selectionModel()->clearSelection();
  clearSelection();
  update();
  coreDataManager::get().setShotClassPtr(nullptr);
  doodleUseFilter(filterState::notFilter);
}

DOODLE_NAMESPACE_E

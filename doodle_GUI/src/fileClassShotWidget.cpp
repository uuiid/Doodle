/*
 * @Author: your name
 * @Date: 2020-10-10 10:26:01
 * @LastEditTime: 2020-10-10 15:58:55
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit  
 * @FilePath: \Doodle\doodle_GUI\src\fileClassShotWidget.cpp
 */
#include "Logger.h"
#include "fileClassShotModel.h"
#include "fileClassShotWidget.h"
#include "src/coreset.h"
#include "src/shot.h"
#include "src/shotClass.h"

#include <QMenu>
#include <QContextMenuEvent>

DOODLE_NAMESPACE_S

/* --------------------------------- 自定义小部件 --------------------------------- */
fileClassShotWidget::fileClassShotWidget(QWidget *parent)
    : QListView(parent),
      p_model_(nullptr),
      p_fileClass_menu(nullptr),
      p_shot(nullptr) {

  setStatusTip(tr("使用右键直接添加部门类型"));

  connect(this, &fileClassShotWidget::clicked,
          this, &fileClassShotWidget::_doodle_fileclass_emit);
}

void fileClassShotWidget::init(const doCore::shotPtr &shot) {
  p_shot = shot;
  p_model_->init(shot);
}

void fileClassShotWidget::insertFileClass() {
  int kRow = selectionModel()->currentIndex().row() + 1;
  p_model_->insertRow(kRow, QModelIndex());

  setCurrentIndex(p_model_->index(kRow));
  edit(p_model_->index(kRow));
}

void fileClassShotWidget::_doodle_fileclass_emit(const QModelIndex &index) {
  emit fileClassShotEmitted(p_model_->dataRow(index));
}

void fileClassShotWidget::contextMenuEvent(QContextMenuEvent *event) {
  p_fileClass_menu = new QMenu(this);

  if (p_shot) {
    auto *action = new QAction(this);

    connect(action, &QAction::triggered,
            this, &fileClassShotWidget::insertFileClass);
    action->setText(tr("添加部门"));
    action->setToolTip(tr("添加本部门"));
    p_fileClass_menu->addAction(action);
  }
  p_fileClass_menu->move(event->globalPos());
  p_fileClass_menu->show();
  DOODLE_LOG_INFO << "显示部门上下文菜单";
}
void fileClassShotWidget::clear() {
  p_model_->clear();
}
void fileClassShotWidget::setModel(QAbstractItemModel *model) {
  auto p_model = dynamic_cast<fileClassShotModel *>(model);
  if (p_model)
    p_model_ = p_model;
  QAbstractItemView::setModel(model);
}

DOODLE_NAMESPACE_E

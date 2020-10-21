//
// Created by teXiao on 2020/10/13.
//

#include <QJsonArray>
#include <QMenu>
#include <QContextMenuEvent>
#include <QHeaderView>

#include "src/mayaArchive.h"

#include "src/filetype.h"
#include "src/shotfilesqlinfo.h"
#include "Logger.h"
#include "shotTableModel.h"

#include "shotTableWidget.h"

#include <QMimeData>
#include <QFileDialog>

DOODLE_NAMESPACE_S
//-----------------------------------自定义shot小部件---------------------------------------------//
shotTableWidget::shotTableWidget(QWidget *parent)
    : QTableView(parent),
      p_model_(),
      p_menu_() {
  setSelectionBehavior(QAbstractItemView::SelectRows);//行选
  setSelectionMode(QAbstractItemView::SingleSelection);//单选
  setAcceptDrops(true);
}
void shotTableWidget::init(const doCore::fileTypePtr &file_type_ptr) {
  p_type_ptr_ = file_type_ptr;
  p_model_->init(file_type_ptr);
  horizontalHeader()->setVisible(true);
  horizontalHeader()->setMinimumHeight(20);
  horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
}
void shotTableWidget::insertShot(const QString &path) {
  DOODLE_LOG_INFO << "提交文件";
  //获得最大版本
  auto version = p_model_->data(p_model_->index(0, 0), Qt::UserRole)
      .toInt();
  //插入新的数据
  p_model_->insertRow(0, QModelIndex());
  auto data = p_model_->data(p_model_->index(0, 4), Qt::UserRole)
      .value<doCore::shotInfoPtr>();
  data->setVersionP(version + 1);
  //创建上传类
  auto file = doCore::mayaArchivePtr::create(data);
  if (path.isEmpty()) return;
  //开始上传
  file->update(path);
  //更新列表
  p_model_->init(p_type_ptr_);

}
void shotTableWidget::contextMenuEvent(QContextMenuEvent *event) {
  p_menu_ = new QMenu(this);
  if (p_type_ptr_) {
    //上传文件
    auto *k_sub_file = new QAction(p_menu_);
    connect(k_sub_file, &QAction::triggered,
            this, &shotTableWidget::getSelectPath);
    k_sub_file->setText(tr("提交文件"));
    k_sub_file->setStatusTip(tr("提交所需要的文件"));

    //打开文件位置


    //导出fbx脚本


    //复制文件目录




    p_menu_->addAction(k_sub_file);
  }

  p_menu_->popup(event->globalPos());
}
void shotTableWidget::clear() {
  p_type_ptr_ = nullptr;
  p_model_->clear();
}
void shotTableWidget::setModel(QAbstractItemModel *model) {
  auto p_model = dynamic_cast<shotTableModel *>(model);
  if (p_model)
    p_model_ = p_model;
  QTableView::setModel(model);
}
void shotTableWidget::dragEnterEvent(QDragEnterEvent *event) {
  if (event->mimeData()->hasUrls()) {
    event->acceptProposedAction();
    enableBorder(true);
  } else
    event->ignore();
//  QAbstractItemView::dragEnterEvent(event);
}
void shotTableWidget::dragLeaveEvent(QDragLeaveEvent *event) {
  enableBorder(false);
//  QAbstractItemView::dragLeaveEvent(event);
}
void shotTableWidget::dragMoveEvent(QDragMoveEvent *event) {
//  QAbstractItemView::dragMoveEvent(event);
}
void shotTableWidget::dropEvent(QDropEvent *event) {
  QAbstractItemView::dropEvent(event);
  if (!event->mimeData()->hasUrls()) return;
  if (event->mimeData()->urls().size() != 1) return;
  auto url = event->mimeData()->urls()[0];
  DOODLE_LOG_INFO << "文件拖入窗口" << url;
  insertShot(url.toLocalFile());
  enableBorder(false);
}
void shotTableWidget::enableBorder(const bool &isEnable) {
  if (isEnable)
    setStyleSheet("border:3px solid #165E23");
  else
    setStyleSheet("");
}
void shotTableWidget::getSelectPath() {
  auto path = QFileDialog::getOpenFileName(this,
                                           tr("提交文件"),
                                           QString(),
                                           "files (*.mb *.ma *.uproject *.max *.fbx *.png *.tga *.jpg))");
  insertShot(path);
}

DOODLE_NAMESPACE_E


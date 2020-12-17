//
// Created by teXiao on 2020/10/13.
//

#include "shotTableWidget.h"

#include <core_doQt.h>
#include <src/toolkit/toolkit.h>
#include <boost/process.hpp>

#include <QContextMenuEvent>
#include <QFileDialog>
#include <QHeaderView>
#include <QJsonArray>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QProgressDialog>
#include <memory>
#include <thread>

#include "Logger.h"
#include "src/shotsWidght/model/shotTableModel.h"
#include <future>
#include <src/toolkit/updataManager.h>
DOODLE_NAMESPACE_S
//-----------------------------------自定义shot小部件---------------------------------------------//
shotTableWidget::shotTableWidget(QWidget *parent)
    : QTableView(parent), p_model_(), p_menu_(), p_type_ptr_() {
  setSelectionBehavior(QAbstractItemView::SelectRows);   //行选
  setSelectionMode(QAbstractItemView::SingleSelection);  //单选
  setAcceptDrops(true);

  connect(this, &shotTableWidget::clicked, this,
          &shotTableWidget::doClickedSlots);
  connect(this, &shotTableWidget::doubleClicked, this,
          &shotTableWidget::doDubledSlots);
}
void shotTableWidget::init() {
  horizontalHeader()->setVisible(true);
  horizontalHeader()->setMinimumHeight(20);
  horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
}
void shotTableWidget::contextMenuEvent(QContextMenuEvent *event) {
  if (!p_menu_) p_menu_ = new QMenu(this);
  p_menu_->clear();
  if (doCore::coreDataManager::get().getShotPtr()) {
    //上传文件
    auto k_sub_file = new QAction();
    connect(k_sub_file, &QAction::triggered, this,
            &shotTableWidget::getSelectPath);
    k_sub_file->setText(tr("提交文件"));
    k_sub_file->setStatusTip(tr("提交所需要的文件"));
    p_menu_->addAction(k_sub_file);
    //上传拍屏
    auto k_sub_fb = new QAction();
    connect(k_sub_fb, &QAction::triggered, this,
            &shotTableWidget::getSelectDir);
    k_sub_fb->setText(tr("提交目录"));
    p_menu_->addAction(k_sub_fb);

    auto k_show_all = new QAction();
    connect(k_show_all, &QAction::triggered, this,
            [=]() { p_model_->showAll(); });
    k_show_all->setText("显示所有");
    p_menu_->addAction(k_show_all);

    if (selectionModel()->hasSelection()) {
      p_menu_->addSeparator();
      auto index = p_model_->index(selectionModel()->currentIndex().row(), 4);
      //打开文件位置
      auto k_openFile = new QAction();
      k_openFile->setText(tr("打开文件所在位置"));
      connect(k_openFile, &QAction::triggered, this, [=] {
        toolkit::openPath(index.data(Qt::UserRole).value<doCore::shotInfoPtr>(),
                          true);
      });
      p_menu_->addAction(k_openFile);
      //复制文件目录到剪切板
      auto k_copyClip = new QAction();
      k_copyClip->setText(tr("复制到剪贴板"));

      connect(k_copyClip, &QAction::triggered, this, [=] {
        toolkit::openPath(index.data(Qt::UserRole).value<doCore::shotInfoPtr>(),
                          false);
      });
      p_menu_->addAction(k_copyClip);

      const auto suffix =
          index.data(Qt::UserRole).value<doCore::shotInfoPtr>()->getSuffixes();
      if (suffix == ".ma" || suffix == ".mb") {
        //导出fbx脚本
        auto k_exportFbx = new QAction();
        k_exportFbx->setText(tr("导出fbx文件"));
        connect(k_exportFbx, &QAction::triggered, this,
                &shotTableWidget::exportFbx);
        p_menu_->addAction(k_exportFbx);
      }
    }
  } else if (doCore::coreDataManager::get().getEpisodesPtr()) {
    if (selectionModel()->hasSelection()) {
      auto index = p_model_->index(selectionModel()->currentIndex().row(), 4);
      //打开文件位置
      auto k_openFile = new QAction();
      k_openFile->setText(tr("打开文件所在位置"));
      connect(k_openFile, &QAction::triggered, this, [=] {
        toolkit::openPath(index.data(Qt::UserRole).value<doCore::shotInfoPtr>(),
                          true);
      });
      p_menu_->addAction(k_openFile);
      //复制文件目录到剪切板
      auto k_copyClip = new QAction();
      k_copyClip->setText(tr("复制到剪贴板"));

      connect(k_copyClip, &QAction::triggered, this, [=] {
        toolkit::openPath(index.data(Qt::UserRole).value<doCore::shotInfoPtr>(),
                          false);
      });
      p_menu_->addAction(k_copyClip);
    }
  }

  p_menu_->popup(event->globalPos());
}

void shotTableWidget::setModel(QAbstractItemModel *model) {
  auto p_model = dynamic_cast<shotTableModel *>(model);
  if (p_model) p_model_ = p_model;
  QTableView::setModel(model);
  init();
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
  if (!event->mimeData()->hasUrls()) return enableBorder(false);
  if (event->mimeData()->urls().size() != 1) return enableBorder(false);
  auto url = event->mimeData()->urls()[0];
  DOODLE_LOG_INFO("文件拖入窗口" << url.toString().toStdString());

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
  auto path = QFileDialog::getOpenFileName(
      this, tr("提交文件"), QString(),
      "files (*.mb *.ma *.uproject *.max *.fbx *.png *.tga *.jpg))");
  if (path.isEmpty()) return;
  if (path.isNull()) return;
  insertShot(path);
}
void shotTableWidget::getSelectDir() {
  auto path =
      QFileDialog::getExistingDirectory(this, tr("提交文件"), QString());
  if (path.isEmpty()) return;
  ;
  if (path.isNull()) return;
  insertShot(path);
}
void shotTableWidget::insertShot(const QString &path) {
  DOODLE_LOG_INFO("提交文件");
  if (path.isEmpty()) return;
  auto pathInfo = QFileInfo(path);
  //插入新的数据
  p_model_->insertRow(0, QModelIndex());
  auto data = p_model_->data(p_model_->index(0, 4), Qt::UserRole)
                  .value<doCore::shotInfoPtr>();

  if (pathInfo.isFile()) {
    if (pathInfo.suffix() == "ma" || pathInfo.suffix() == "mb") {  // maya文件
      data->setShotType(doCore::shotType::findShotType("Animation", true));
      submitMayaFile(data, path);
    } else if (pathInfo.suffix() == "mp4" || pathInfo.suffix() == "avi" ||
               pathInfo.suffix() == "mov") {  //拖拽文件(拍屏已经是视频文件)
      data->setShotType(doCore::shotType::findShotType("flipbook", true));
      submitFBFile(data, path);
    }
  } else if (pathInfo.isDir()) {  //拖动路径(拍屏所在路径)
    if (QDir(path).isEmpty()) {
      p_model_->removeRow(0, QModelIndex());
      return;
    }
    data->setShotType(doCore::shotType::findShotType("flipbook", true));
    submitFBFile(data, path);
  }
  //更新列表
  p_model_->init();
}
void shotTableWidget::exportFbx() {
  //是否选择导出物体
  if (!selectionModel()->hasSelection()) return;
  //获得选择数据
  auto index = p_model_->index(selectionModel()->currentIndex().row(), 4);

  auto data = index.data(Qt::UserRole).value<doCore::shotInfoPtr>();
  if (!data) return;

  p_model_->insertRow(0, QModelIndex());

  auto export_data = p_model_->data(p_model_->index(0, 4), Qt::UserRole)
                         .value<doCore::shotInfoPtr>();

  //创建上传类
  auto k_fileexport = std::make_shared<doCore::mayaArchiveShotFbx>(export_data);
  //开始导出
  auto fun = std::async(std::launch::async, [=]() -> bool {
    auto result = k_fileexport->update(data->getFileList().front());
    // if (!result) emit exportFbxError(DOTOS(data->getFileList().front().filename().generic_string()));
    return result;
  });
  updataManager::get().addQueue(fun, "正在上传中", 2000);
  updataManager::get().run();

  //更新列表
  p_model_->init();
}

void shotTableWidget::doClickedSlots(const QModelIndex &index) {
  auto info = index.data(Qt::UserRole).value<doCore::shotInfoPtr>();
  if (info) doCore::coreDataManager::get().setShotInfoPtr(info);
}

void shotTableWidget::doDubledSlots(const QModelIndex &index) {
  auto info = index.data(Qt::UserRole).value<doCore::shotInfoPtr>();
  if (info) {
    auto path = info->getFileList().front();
    toolkit::openPath(path);
  }
}
void shotTableWidget::submitMayaFile(doCore::shotInfoPtr &info_ptr,
                                     const QString &path) {
  auto file = std::make_shared<doCore::mayaArchive>(info_ptr);
  auto fun = std::async(std::launch::async,
                        [=]() { return file->update(path.toStdString()); });
  updataManager::get().addQueue(fun, "正在上传中", 100);
  updataManager::get().run();
}
void shotTableWidget::submitFBFile(doCore::shotInfoPtr &info_ptr,
                                   const QString &path) {
  auto k_movie = std::make_shared<doCore::moveShotA>(info_ptr);
  std::future<bool> k_fu;
  k_fu = std::async(std::launch::async,
                    [=]() {
                      auto result = k_movie->update({path.toStdString()});
                      return result;
                    });
  updataManager::get().addQueue(k_fu, "正在上传中", 1000);
  updataManager::get().run();
}

DOODLE_NAMESPACE_E

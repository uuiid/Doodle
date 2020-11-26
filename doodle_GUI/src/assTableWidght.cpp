//
// Created by teXiao on 2020/10/17.
//
#include <src/assTableWidght.h>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <core_doQt.h>
#include <future>
#include <Logger.h>
#include <QContextMenuEvent>
#include <QHeaderView>
#include <QMenu>
#include <QMimeData>
#include <QtCore/QTimer>
#include <QtWidgets/qapplication.h>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/qmessagebox.h>
#include <QtWidgets/QProgressDialog>
#include <QtWidgets/QPushButton>
#include <QtWidgets/qlistwidget.h>
#include <src/assTableModel.h>
#include <src/imageArchive.h>
#include <src/toolkit.h>
#include <src/updataManager.h>
#include <string>
#include <src/shotEpsListWidget.h>
#include <ObjIdlbase.h>
DOODLE_NAMESPACE_S
assTableWidght::assTableWidght(QWidget *parent)
    : QTableView(parent),
      p_menu_(nullptr),
      p_model_(nullptr) {
  setSelectionBehavior(QAbstractItemView::SelectRows);   //行选
  setSelectionMode(QAbstractItemView::SingleSelection);  //单选
  setSelectionBehavior(QAbstractItemView::SelectRows);
  setShowGrid(false);
  setFrameShape(QFrame::NoFrame);
  setAcceptDrops(true);
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
void assTableWidght::insertAss(const QString &path) {
  DOODLE_LOG_INFO << "获得路径: " << path;
  auto pathInfo = QFileInfo(path);
  boost::regex reMaya("m[ab]");
  boost::regex reUe4("uproject");

  p_model_->insertRow(0, QModelIndex());
  auto data = p_model_->data(p_model_->index(0, 4), Qt::UserRole)
      .value<doCore::assInfoPtr>();
  QMessageBox msgBox;
  auto text_info = QInputDialog::getText(this, tr("请输入备注"),
                                         tr("请输入文件备注"));
  data->setInfoP(text_info.toStdString());
  DOODLE_LOG_INFO << pathInfo.suffix();

  if (boost::regex_match(pathInfo.suffix().toStdString(), reMaya)) {
    //maya文件
    msgBox.setText(tr("请选择类型"));
    auto modelFile = msgBox.addButton("模型文件", QMessageBox::AcceptRole);
    auto rig = msgBox.addButton("绑定文件", QMessageBox::AcceptRole);
    auto modelFile_low = msgBox.addButton("低模文件", QMessageBox::AcceptRole);
    auto noButten = msgBox.addButton("取消", QMessageBox::NoRole);

    msgBox.exec();

    if (msgBox.clickedButton() == modelFile) {
      data->setAssType(doCore::assType::findType(doCore::assType::e_type::scenes, true));
    } else if (msgBox.clickedButton() == rig) {
      data->setAssType(doCore::assType::findType(doCore::assType::e_type::rig, true));
    } else if (msgBox.clickedButton() == modelFile_low) {
      data->setAssType(doCore::assType::findType(doCore::assType::e_type::scenes_low, true));
    } else if (msgBox.clickedButton() == noButten) {
      p_model_->removeRow(0);
      return;
    }
    auto maya_archive = std::make_shared<doCore::mayaArchive>(data);

    auto future = std::async(std::launch::async, [=]() {
      return maya_archive->update(path.toStdString());
    });
    //打开后台传输
    updataManager::get().addQueue(future, "正在上传中", 100);
    updataManager::get().run();

  } else if (boost::regex_search(pathInfo.suffix().toStdString(), reUe4)) {
    //ue4文件
    data->setAssType(doCore::assType::findType(doCore::assType::e_type::UE4, true));
    auto ue4_archice = std::make_shared<doCore::ueArchive>(data);
    ue4_archice->update(path.toStdString());
  } else {
    //图片文件
    data->setAssType(doCore::assType::findType(doCore::assType::e_type::screenshot, true));
    p_model_->removeRow(0);
    //    auto image_archice = std::make_shared<doCore::imageArchive>(data);
    return;
  }
  p_model_->filter(false);
}
void assTableWidght::contextMenuEvent(QContextMenuEvent *event) {
  //只有在菜单指针为空 并且 持有上一级的的情况下才进行显示菜单
  if (p_menu_) {
    p_menu_->clear();
  } else {
    p_menu_ = new QMenu(this);
  }
  auto sub_file = new QAction(p_menu_);
  sub_file->setText(tr("提交文件"));
  connect(sub_file, &QAction::triggered,
          this, &assTableWidght::openFileDialog);
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

    if (index.data(Qt::UserRole).value<doCore::assInfoPtr>()->getAssType()
        == doCore::assType::findType(doCore::assType::e_type::UE4, false)) {
      auto k_createDir = new QAction();
      k_createDir->setText("创建灯光文件夹");
      connect(k_createDir, &QAction::triggered,
              this, &assTableWidght::createLightDir);
      p_menu_->addAction(k_createDir);
    }
  }

  p_menu_->move(event->globalPos());
  p_menu_->show();
}
void assTableWidght::openFileDialog() {
  auto path = QFileDialog::getOpenFileName(
      this, tr("提交文件"), QString(),
      "files (*.mb *.ma *.uproject *.max *.fbx *.png *.tga *.jpg))");
  if (path.isEmpty()) return;
  if (path.isNull()) return;
  insertAss(path);
}
void assTableWidght::dropEvent(QDropEvent *event) {
  QAbstractItemView::dropEvent(event);
  if (!event->mimeData()->hasUrls()) return enableBorder(false);
  if (event->mimeData()->urls().size() != 1) return enableBorder(false);
  auto url = event->mimeData()->urls()[0];
  DOODLE_LOG_INFO << "文件拖入窗口" << url;

  const QFileInfo &kFileInfo = QFileInfo(url.toLocalFile());
  enableBorder(false);
  if (kFileInfo.exists()) {
    insertAss(url.toLocalFile());
  }
  //  QAbstractItemView::dropEvent(event);
}
void assTableWidght::dragMoveEvent(QDragMoveEvent *event) {
  //  QAbstractItemView::dragMoveEvent(event);
}
void assTableWidght::dragLeaveEvent(QDragLeaveEvent *event) {
  enableBorder(false);
  //  QAbstractItemView::dragLeaveEvent(event);
}
void assTableWidght::dragEnterEvent(QDragEnterEvent *event) {
  if (event->mimeData()->hasUrls()) {
    event->acceptProposedAction();
    enableBorder(true);
  } else
    event->ignore();
  //  QAbstractItemView::dragEnterEvent(event);
}
void assTableWidght::enableBorder(const bool &isEnable) {
  if (isEnable)
    setStyleSheet("border:3px solid #165E23");
  else
    setStyleSheet("");
}
void assTableWidght::createLightDir() {
  auto widghtList = qApp->allWidgets();
  shotEpsListWidget *epsListModle = nullptr;

  for (auto &wid : widghtList) {
    if (!epsListModle) {
      epsListModle = wid->findChild<shotEpsListWidget *>("p_episodes_list_widget_");
    } else {
      break;
    }
  }
  if (epsListModle) {
    if (epsListModle->selectionModel()->hasSelection()) {
      auto eps_ptr = epsListModle->selectionModel()->currentIndex().data(Qt::UserRole).value<doCore::episodesPtr>();
      
      auto butten = QMessageBox::question(this, tr("注意"), tr("将添加 %1 集数到共享盘").arg(
          eps_ptr->getEpisdes(), 0, 10, QLatin1Char('0')
      ), QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

      if (butten == QMessageBox::Yes) {
        auto uesyn = std::make_shared<doCore::ueSynArchive>();
        auto eps_ptr = epsListModle->selectionModel()->currentIndex().data(Qt::UserRole).value<doCore::episodesPtr>();
        uesyn->makeDir(eps_ptr);
      }
    } else {
      QMessageBox::warning(this, tr("注意"), tr("请选择集数"));
    }
  }


  //auto epsList = new QInputDialog::getItem(this, tr("选择创建集数"), tr("集数"));
  //auto epsListWidght = new QListView(this);
  //epsListWidght->setWindowFlag(Qt::Dialog);
  //epsListWidght->setModel(epsListModle);
  //epsListWidght->show();

}
DOODLE_NAMESPACE_E
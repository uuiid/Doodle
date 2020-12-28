//
// Created by teXiao on 2020/10/17.
//
//这里开始禁用clang-format排序

// clang-format off
#include "assTableWidght.h"
#include <src/toolkit/updataManager.h>  //这个必须往上放 这样才不会出错

// clang-format on

#include <core_doQt.h>
#include <boost/regex.hpp>
#include <boost/process.hpp>
#include <Logger.h>
#include <ObjIdlbase.h>
#include <QtWidgets/qapplication.h>
#include <QtWidgets/qlistwidget.h>
#include <QtWidgets/qmessagebox.h>

#include <src/assetsWidget/model/assTableModel.h>
#include <src/shotsWidght/veiw/shotEpsListWidget.h>
#include <src/toolkit/toolkit.h>

#include <QContextMenuEvent>
#include <QHeaderView>
#include <QMenu>
#include <QMimeData>
#include <QtCore/QTimer>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QProgressDialog>
#include <QtWidgets/QPushButton>

#include <future>
#include <string>

DOODLE_NAMESPACE_S
assTableWidght::assTableWidght(QWidget *parent)
    : QTableView(parent), p_menu_(nullptr) {
  setSelectionBehavior(QAbstractItemView::SelectRows);   //行选
  setSelectionMode(QAbstractItemView::SingleSelection);  //单选
  setSelectionBehavior(QAbstractItemView::SelectRows);
  setShowGrid(false);
  setFrameShape(QFrame::NoFrame);
  setAcceptDrops(true);
  connect(this, &assTableWidght::clicked, this,
          &assTableWidght::doClickedSlots);
  connect(this, &assTableWidght::doubleClicked, this,
          &assTableWidght::doDubledSlots);
}
void assTableWidght::setModel(QAbstractItemModel *model) {
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
  DOODLE_LOG_INFO("获得路径: " << path.toStdString());
  auto pathInfo = QFileInfo(path);
  static boost::regex reMaya("m[ab]");
  static boost::regex reUe4("uproject");
  static boost::regex reImage(R"((jpe?g|png|tga))");
  model()->insertRow(0, QModelIndex());
  auto data = model()->data(model()->index(0, 4), Qt::UserRole).value<assInfoPtr>();
  //选择提示
  QMessageBox msgBox;
  auto text_info = QInputDialog::getText(this, tr("请输入备注"), tr("请输入文件备注"));

  data->setInfoP(text_info.toStdString());
  DOODLE_LOG_INFO(pathInfo.suffix().toStdString());
  if (pathInfo.isDir()) {
    if (pathInfo.dir().isEmpty()) {
      model()->removeRow(0);
      return;
    } else {
      auto image_file = msgBox.addButton("贴图文件", QMessageBox::AcceptRole);
      auto noButten   = msgBox.addButton("取消", QMessageBox::NoRole);
      msgBox.exec();
      if (msgBox.clickedButton() == image_file) {
        data->setAssType(
            assType::findType(assType::e_type::screenshot, true));
        data                = std::get<assInfoPtr>(data->findSimilar());
        auto k_imageArchive = std::make_shared<imageArchive>(data);
        k_imageArchive->update(path.toStdString());

      } else {
        model()->removeRow(0);
        return;
      }
    }

  } else if (pathInfo.isFile()) {  //确认是文件的情况下//我们测试文件类型
    if (boost::regex_match(pathInfo.suffix().toStdString(), reMaya)) {
      // maya文件
      msgBox.setText(tr("请选择类型"));
      auto modelFile     = msgBox.addButton("模型文件", QMessageBox::AcceptRole);
      auto rig           = msgBox.addButton("绑定文件", QMessageBox::AcceptRole);
      auto modelFile_low = msgBox.addButton("低模文件", QMessageBox::AcceptRole);
      auto noButten      = msgBox.addButton("取消", QMessageBox::NoRole);

      msgBox.exec();

      if (msgBox.clickedButton() == modelFile) {
        data->setAssType(assType::findType(assType::e_type::scenes, true));
      } else if (msgBox.clickedButton() == rig) {
        data->setAssType(assType::findType(assType::e_type::rig, true));
      } else if (msgBox.clickedButton() == modelFile_low) {
        data->setAssType(assType::findType(assType::e_type::scenes_low, true));
      } else if (msgBox.clickedButton() == noButten) {
        model()->removeRow(0);
        return;
      }
      data              = std::get<assInfoPtr>(data->findSimilar());
      auto maya_archive = std::make_shared<mayaArchive>(data);

      auto future = std::async(std::launch::async, [=]() -> bool {
        auto result = maya_archive->update(path.toStdString());
        return result;
      });
      //打开后台传输
      updataManager::get().addQueue(future, "正在上传中", 100);
      updataManager::get().run();

    } else if (boost::regex_search(pathInfo.suffix().toStdString(), reUe4)) {  // ue4文件
      data->setAssType(
          assType::findType(assType::e_type::UE4, true));
      auto ue4_archice = std::make_shared<ueArchive>(data);
      ue4_archice->update(path.toStdString());
    } else {
      model()->removeRow(0);
      //    auto image_archice = std::make_shared< imageArchive>(data);
      return;
    }
  } else {
    model()->removeRow(0);
    //    auto image_archice = std::make_shared< imageArchive>(data);
    return;
  }
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
  connect(sub_file, &QAction::triggered, this, &assTableWidght::openFileDialog);
  p_menu_->addAction(sub_file);

  if (selectionModel()->hasSelection()) {
    auto index      = model()->index(selectionModel()->currentIndex().row(),
                                4);  //获得模型索引
    auto k_openFile = new QAction();
    k_openFile->setText("打开文件所在位置");
    connect(k_openFile, &QAction::triggered, this, [=] {
      toolkit::openPath(index.data(Qt::UserRole).value<assInfoPtr>(),
                        true);
    });
    p_menu_->addAction(k_openFile);

    auto k_deleteFile = new QAction();
    k_deleteFile->setText("删除(小心 别错)");
    connect(k_deleteFile, &QAction::triggered, this,
            &assTableWidght::deleteSQLFile);
    p_menu_->addAction(k_deleteFile);

    if (index.data(Qt::UserRole).value<assInfoPtr>()->getAssType() ==
        assType::findType(assType::e_type::UE4, false)) {
      auto k_createDir = new QAction();
      k_createDir->setText("创建灯光文件夹");
      connect(k_createDir, &QAction::triggered, this,
              &assTableWidght::createLightDir);
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
  DOODLE_LOG_INFO("文件拖入窗口" << url.toString().toStdString());

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
  auto widghtList                 = qApp->allWidgets();
  shotEpsListWidget *epsListModle = nullptr;

  for (auto &wid : widghtList) {
    if (!epsListModle) {
      epsListModle =
          wid->findChild<shotEpsListWidget *>("p_episodes_list_widget_");
    } else {
      break;
    }
  }
  if (epsListModle) {
    if (epsListModle->selectionModel()->hasSelection()) {
      auto eps_ptr = epsListModle->selectionModel()
                         ->currentIndex()
                         .data(Qt::UserRole)
                         .value<episodesPtr>();

      auto ass_ptr = selectionModel()
                         ->currentIndex()
                         .data(Qt::UserRole)
                         .value<assInfoPtr>();
      auto file_exit = true;
      if (ass_ptr) {
        file_exit = boost::filesystem::exists(
            coreSet::getSet().getPrjectRoot() /
            ass_ptr->getFileList().front());
      }
      if (!file_exit) {
        QMessageBox::warning(this, tr("注意"),
                             tr("没有再目录中找到这个文件,请重新提交到数据库"));
        return;
      }

      auto butten = QMessageBox::question(
          this, tr("注意"),
          tr("将添加 %1 集数到共享盘")
              .arg(eps_ptr->getEpisdes(), 0, 10, QLatin1Char('0')),
          QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

      if (butten == QMessageBox::Yes) {
        auto uesyn  = std::make_shared<ueSynArchive>();
        auto future = std::async(std::launch::async, [=]() -> bool {
          return uesyn->makeDir(eps_ptr);
        });
        updataManager::get().addQueue(future, "正在复制文件", 1000);
        updataManager::get().run();
      }
    } else {
      QMessageBox::warning(this, tr("注意"), tr("请选择集数"));
    }
  }
}
void assTableWidght::doClickedSlots(const QModelIndex &index) {
  auto assinfo = index.data(Qt::UserRole).value<assInfoPtr>();
  if (assinfo) coreDataManager::get().setAssInfoPtr(assinfo);
}

void assTableWidght::doDubledSlots(const QModelIndex &index) {
  auto assinfo = index.data(Qt::UserRole).value<assInfoPtr>();
  if (assinfo) {
    if (index.column() != 1) {
      auto path = assinfo->getFileList().front();
      toolkit::openPath(path);
    }
  }
}

void assTableWidght::deleteSQLFile() {
  if (!selectionModel()->hasSelection()) return;
  auto str_delete = QInputDialog::getText(this, tr("输入delete删除条目"),
                                          tr("输入: "), QLineEdit::Password);
  if (str_delete == "delete")
    model()->removeRow(selectionModel()->currentIndex().row());
}
DOODLE_NAMESPACE_E
//
// Created by teXiao on 2020/10/17.
//
#include <src/assTableWidght.h>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <core_doQt.h>
#include <Logger.h>
#include <QContextMenuEvent>
#include <QHeaderView>
#include <QMenu>
#include <QtWidgets/qmessagebox.h>
#include <src/assTableModel.h>
#include <src/Toolkit.h>
#include <string>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QInputDialog>
#include <src/imageArchive.h>

#include <future>
#include <QtWidgets/QProgressDialog>
#include <QtCore/QTimer>
#include <src/updataManager.h>
DOODLE_NAMESPACE_S
assTableWidght::assTableWidght(QWidget *parent)
    : QTableView(parent),
      p_menu_(nullptr),
      p_model_(nullptr),
      p_updataFtpQueue(),
      p_timer_(nullptr) {
  setSelectionBehavior(QAbstractItemView::SelectRows);   //行选
  setSelectionMode(QAbstractItemView::SingleSelection);  //单选
  setSelectionBehavior(QAbstractItemView::SelectRows);
  setShowGrid(false);
  setFrameShape(QFrame::NoFrame);
  p_timer_ = new QTimer(this);
  connect(p_timer_,&QTimer::timeout,
          this,&assTableWidght::chickUpdataQueue);
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
      data->setAssType(doCore::assType::findType("scenes", true));
    } else if (msgBox.clickedButton() == rig) {
      data->setAssType(doCore::assType::findType("rig", true));
    } else if (msgBox.clickedButton() == modelFile_low) {
      data->setAssType(doCore::assType::findType("_low", true));
    } else if (msgBox.clickedButton() == noButten) {
      p_model_->removeRow(0);
      return;
    }
    auto maya_archive = std::make_shared<doCore::mayaArchive>(data);
//    maya_archive->update(path.toStdString());
//    auto updata = std::bind((bool (doCore::ueArchive::*)(const doCore::dpath &)) &doCore::ueArchive::update,
//                            maya_archive,
//                            path.toStdString());
    auto pro = new QProgressDialog(this);
    pro->setLabelText("正在上传中");
    pro->setMinimum(0);
    pro->setMaximum(100);
    pro->setValue(1);
    auto test = std::async(std::launch::async,[=](){
     return maya_archive->update(path.toStdString());
    });
    p_updataFtpQueue.emplace_back(std::move(test), pro);
    if (!p_timer_->isActive()){
      p_timer_->start(2000);
    };
    pro->show();

  } else if (boost::regex_match(pathInfo.suffix().toStdString(), reUe4)) {
    //ue4文件
    data->setAssType(doCore::assType::findType("_UE4", true));
    auto ue4_archice = std::make_shared<doCore::ueArchive>(data);
  } else {
    //图片文件
//    data->setAssType(doCore::assType::findType("sourceimages",true));
//    auto image_archice = std::make_shared<doCore::imageArchive>(data);
    return;
  }
  return;
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
void assTableWidght::chickUpdataQueue() {
  for (const auto &item : p_updataFtpQueue) {
    if (item.first.wait_for(std::chrono::microseconds(1)) == std::future_status::ready) {
      item.second->setValue(100);
//      item.second->cancel();
    } else {
      if (item.second->value() < 100)
        item.second->setValue(item.second->value() + 1);
    }
  }
  p_updataFtpQueue.erase(
      std::remove_if(p_updataFtpQueue.begin(),p_updataFtpQueue.end(),
                     [this](std::pair<std::future<bool>,QProgressDialog *> &part){
        return part.first.wait_for(std::chrono::microseconds(1)) == std::future_status::ready;
      }),p_updataFtpQueue.end()
      );
  if (p_updataFtpQueue.empty())
    p_timer_->stop();
}

DOODLE_NAMESPACE_E
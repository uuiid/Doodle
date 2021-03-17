#include <lib/ui/view/MotionView.h>

#include <lib/kernel/Exception.h>
#include <lib/kernel/MotionFile.h>

#include <lib/ui/model/TreeDirItem.h>
#include <lib/ui/model/MotionModel.h>

#include <QtWidgets/qmenu.h>
#include <QtGui/qevent.h>
#include <QtWidgets/qinputdialog.h>
#include <QtWidgets/qmessagebox.h>

namespace doodle::motion::ui {
MotionView::MotionView(QWidget* parent)
    : QListView(parent),
      p_TreeDirItem(),
      sig_chickItem() {
  this->setViewMode(QListView::ViewMode::IconMode);
  this->setFlow(QListView::Flow::LeftToRight);
  this->setUniformItemSizes(true);
  // this->setGridSize(QSize{128, 128});
  connect(this, &MotionView::clicked, this, &MotionView::doodleChicked);
}

void MotionView::setTreeNode(const decltype(p_TreeDirItem)& item) {
  p_TreeDirItem = item;
}

void MotionView::contextMenuEvent(QContextMenuEvent* event) {
  if (!p_TreeDirItem) return;

  auto menu = new QMenu(this);

  auto k_create_fbx = menu->addAction(tr("创建动作"));
  connect(k_create_fbx, &QAction::triggered,
          this, [this]() {
            auto k_path = this->p_TreeDirItem->Dir(false);
            this->createFbxAction(k_path);
          });
  if (selectionModel()->hasSelection()) {
    auto k_import_fbx = menu->addAction(tr("导入文件"));
    connect(k_import_fbx, &QAction::triggered, this, &MotionView::importFbxAction);

    menu->addSeparator();

    auto k_create_icon = menu->addAction(tr("更新图标"));
    connect(k_create_icon, &QAction::triggered, this, &MotionView::updateIcon);
    auto k_create_video = menu->addAction(tr("更新视频"));
    connect(k_create_video, &QAction::triggered, this, &MotionView::updateVideo);
  }
  menu->move(event->globalPos());
  menu->show();
}

void MotionView::createFbxAction(const FSys::path& path) {
  auto k_sort_model = dynamic_cast<QSortFilterProxyModel*>(this->model());
  if (!k_sort_model) return;

  auto k_model = dynamic_cast<MotionModel*>(k_sort_model->sourceModel());
  if (!k_model) return;

  auto k_name = QInputDialog::getText(this, tr("请输入名称 "), tr("名称:"));

  if (k_name.isEmpty()) return;

  auto k_FbxFile = std::make_shared<kernel::MotionFile>();
  k_FbxFile->setTitle(k_name.toStdString());
  try {
    k_FbxFile->createFbxFile(path);
  } catch (const FbxFileError& err) {
    QMessageBox::warning(this, QString::fromUtf8("注意: "), tr(err.what()));
  } catch (const MayaError& err) {
    QMessageBox::warning(this, QString::fromUtf8("注意: "), tr(err.what()));
  } catch (const MayaNullptrError& err) {
    QMessageBox::warning(this, QString::fromUtf8("注意: "), tr(err.what()));
  } catch (const FFmpegError& err) {
    QMessageBox::warning(this, QString::fromUtf8("注意: "), tr(err.what()));
  } catch (const NotFileError& err) {
    QMessageBox::warning(this, QString::fromUtf8("注意: "), tr(err.what()));
  }
  k_model->insertData(0, k_FbxFile);
}

void MotionView::updateIcon() {
  auto k_selectModel = selectionModel();
  if (!k_selectModel->hasSelection()) return;

  auto k_data = k_selectModel->currentIndex().data(Qt::UserRole).value<kernel::MotionFile*>();
  if (!k_data) QMessageBox::warning(this, QString::fromUtf8("注意: "), tr("没有找到选择"));
  try {
    k_data->createIconFile();
  } catch (const NotFileError& err) {
    QMessageBox::warning(this, QString::fromUtf8("注意: "), tr(err.what()));
  }
}

void MotionView::updateVideo() {
  auto k_selectModel = selectionModel();
  if (!k_selectModel->hasSelection()) return;

  auto k_data = k_selectModel->currentIndex().data(Qt::UserRole).value<kernel::MotionFile*>();
  if (!k_data) QMessageBox::warning(this, QString::fromUtf8("注意: "), tr("没有找到选择"));
  try {
    k_data->createVideoFile();
  } catch (const NotFileError& err) {
    QMessageBox::warning(this, QString::fromUtf8("注意: "), tr(err.what()));
  }
}

void MotionView::importFbxAction() {
  auto k_selectModel = selectionModel();
  if (!k_selectModel->hasSelection()) return;

  auto k_data = k_selectModel->currentIndex().data(Qt::UserRole).value<kernel::MotionFile*>();
  if (!k_data) QMessageBox::warning(this, QString::fromUtf8("注意: "), tr("没有找到选择"));

  try {
    k_data->importFbxFile();
  } catch (const FbxFileError& err) {
    QMessageBox::warning(this, QString::fromUtf8("注意: "), tr(err.what()));
  }
}

void MotionView::doodleChicked(const QModelIndex& index) {
  if (!index.isValid()) return;
  auto k_user_data = index.data(Qt::UserRole);

  if (!k_user_data.canConvert<kernel::MotionFile*>())
    return;

  auto k_data = k_user_data.value<kernel::MotionFile*>();
  if (!k_data)
    return;
  sig_chickItem(k_data->shared_from_this());
}
}  // namespace doodle::motion::ui
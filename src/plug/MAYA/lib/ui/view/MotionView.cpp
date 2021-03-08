#include <lib/ui/view/MotionView.h>

#include <lib/kernel/Exception.h>

#include <lib/ui/model/TreeDirItem.h>
#include <lib/kernel/MotionFile.h>
#include <lib/ui/model/MotionModel.h>

#include <QtWidgets/qmenu.h>
#include <QtGui/qevent.h>
#include <QtWidgets/qinputdialog.h>
#include <QtWidgets/qmessagebox.h>

namespace doodle::motion::ui {
MotionView::MotionView(QWidget* parent)
    : QListView(parent),
      p_TreeDirItem() {
  this->setViewMode(QListView::ViewMode::IconMode);
  this->setFlow(QListView::Flow::LeftToRight);
}

void MotionView::setTreeNode(const decltype(p_TreeDirItem)& item) {
  p_TreeDirItem = item;
}

void MotionView::contextMenuEvent(QContextMenuEvent* event) {
  if (!p_TreeDirItem) return;

  auto menu         = new QMenu(this);
  auto k_create_fbx = menu->addAction(tr("创建动作"));
  connect(k_create_fbx, &QAction::triggered,
          this, [this]() {
            auto k_path = this->p_TreeDirItem->Dir(false);
            this->createFbxAction(k_path);
          });
  menu->move(event->globalPos());
  menu->show();
}

void MotionView::createFbxAction(const FSys::path& path) {
  auto k_model = dynamic_cast<MotionModel*>(this->model());
  if (!k_model) return;

  auto k_name = QInputDialog::getText(this, tr("请输入名称 "), tr("名称:"));

  if (k_name.isEmpty()) return;

  auto k_FbxFile = std::make_shared<kernel::MotionFile>();
  k_FbxFile->setTitle(k_name.toStdString());
  try {
    k_FbxFile->createFbxFile(path);
  } catch (const FbxFileError& err) {
    QMessageBox::warning(this, QString::fromUtf8("注意: "), tr(err.what()));
  }

  k_model->insertData(0, k_FbxFile);
}

}  // namespace doodle::motion::ui
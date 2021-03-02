#include <lib/ui/view/MotionView.h>
#include <lib/kernel/FbxExport/FbxExport.h>
#include <QtWidgets/qmenu.h>
namespace doodle::motion::ui {
MotionView::MotionView(QWidget* parent) {
  this->setViewMode(QListView::ViewMode::IconMode);
  this->setFlow(QListView::Flow::LeftToRight);
}

void MotionView::contextMenuEvent(QContextMenuEvent* event) {
  auto menu = new QMenu(this);

  auto create = menu->addAction(tr("创建动作"));
  // connect(create, QAction::triggered, this, []() { kernel::FbxExport::FbxExportMEL(""); });
}

}  // namespace doodle::motion::ui
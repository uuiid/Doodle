#include <lib/ui/view/MotionView.h>

namespace doodle::motion::ui {
MotionView::MotionView(QWidget* parent) {
  this->setViewMode(QListView::ViewMode::IconMode);
  this->setFlow(QListView::Flow::LeftToRight);
}

}  // namespace doodle::motion::ui
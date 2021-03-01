#pragma once

#include <MotionGlobal.h>
#include <QtWidgets/QListView>

namespace doodle::motion::ui {
class MotionView : public QListView {
 private:
  Q_OBJECT
 public:
  MotionView(QWidget* parent = nullptr);
};

}  // namespace doodle::motion::ui
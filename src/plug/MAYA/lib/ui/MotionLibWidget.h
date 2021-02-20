#pragma once

#include <lib/MotionGlobal.h>
#include <QtWidgets/qwidget.h>

namespace doodle::motion::ui {
class MotionLibWidget : public QWidget {
  Q_OBJECT
 public:
  explicit MotionLibWidget(QWidget *parent = nullptr);

 private:
  
};

}  // namespace doodle::motion::ui
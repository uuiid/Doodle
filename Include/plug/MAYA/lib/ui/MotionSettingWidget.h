#pragma once

#include <QtCore/qobject.h>
#include <QtWidgets/qwidget.h>

namespace doodle::motion::ui {
class MotionSettingWidget : public QWidget {
  Q_OBJECT
 public:
  explicit MotionSettingWidget(QWidget *parent = nullptr);

 Q_SIGNALS:
  void ReturnUp();

 private:
};

}  // namespace doodle::motion::ui
#pragma once

#include <lib/MotionGlobal.h>

#include <QtCore/qobject.h>
#include <QtWidgets/qwidget.h>
#include <QtWidgets/qmainWindow.h>

namespace doodle::motion::ui {
class MOTIONGLOBAL_API MotionMainUI : public QMainWindow {
  Q_OBJECT
 public:
  explicit MotionMainUI(QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());

 private:
  /* data */
};

}  // namespace doodle::motion::ui
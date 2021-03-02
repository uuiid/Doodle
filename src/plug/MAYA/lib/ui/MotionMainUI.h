#pragma once

#include <memory>
#include <lib/MotionGlobal.h>

#include <QtCore/qobject.h>
#include <QtWidgets/qwidget.h>
#include <QtWidgets/qmainWindow.h>

class QGridLayout;

namespace doodle::motion::ui {
class MotionSettingWidget;

class MOTIONGLOBAL_API MotionMainUI : public QMainWindow {
  Q_OBJECT
 public:
  explicit MotionMainUI(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());

  void setMotionLib();
  void openMotionLib();

 private:
  QGridLayout* p_layout;
  QWidget* p_centralWidget;
  MotionSettingWidget* p_setting_widget;
};

}  // namespace doodle::motion::ui
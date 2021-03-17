#pragma once

#include <memory>
#include <lib/MotionGlobal.h>

#include <QtCore/qobject.h>
#include <QtWidgets/qwidget.h>
#include <QtWidgets/qmainWindow.h>

class QGridLayout;

namespace doodle::motion::ui {
class MotionSettingWidget;
class MotionLibWidget;

class MOTIONGLOBAL_API MotionMainUI : public QMainWindow {
  Q_OBJECT
  QGridLayout* p_layout;
  QWidget* p_centralWidget;
  MotionSettingWidget* p_setting_widget;
  MotionLibWidget* p_motion;

 public:
  explicit MotionMainUI(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());
  ~MotionMainUI();
  void openSettingMotionLib();
  void openMotionLib();

 private:
  void createMenu();
};

}  // namespace doodle::motion::ui
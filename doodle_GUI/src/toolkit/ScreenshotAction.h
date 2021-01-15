#pragma once

#include <doodle_global.h>
#include <QtWidgets/qdialog.h>
DOODLE_NAMESPACE_S

class ScreenshotAction : public QDialog {
  Q_OBJECT
 public:
  ScreenshotAction(QWidget *parent = nullptr);

 private:
};
DOODLE_NAMESPACE_E
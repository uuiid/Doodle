#pragma once

#include <doodle_global.h>
#include <QtWidgets/qwidget.h>

class QLabel;
class QPushButton;

DOODLE_NAMESPACE_S

class ScreenshotWidght : public QWidget {
  Q_OBJECT
 public:
  ScreenshotWidght(QWidget* parent = nullptr);

  void createScreenshot();
  void showImage();

 private:
  QPushButton* p_butten;
  QLabel* p_image;
  ScreenshotAction* p_action;
  std::weak_ptr<fileSqlInfo> p_file_archive;
};

DOODLE_NAMESPACE_E
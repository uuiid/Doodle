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

  virtual void createScreenshot();
  void showImage();

  void disableButten(bool disable);
  // virtual void setIndexInfo(const std::shared_ptr<coresqldata>& typeptr) = 0;
 private:
  QLabel* p_image;
  ScreenshotAction* p_action;
  QPushButton* p_butten;

 protected:
  std::weak_ptr<fileSqlInfo> p_file_archive;
};

DOODLE_NAMESPACE_E
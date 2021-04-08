#pragma once

#include <doodle_GUI/doodle_global.h>


#include <QtWidgets/qdialog.h>
#include <QtGui/qbitmap.h>

DOODLE_NAMESPACE_S

class ScreenshotAction : public QDialog {
  Q_OBJECT
 public:
  ScreenshotAction(QWidget *parent = nullptr);

  void screenShot(const fileSys::path &save_path);

 protected:
  void paintEvent(QPaintEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;

 private:
  bool p_isDrawing_b;
  QScreen *p_screen;
  QPoint p_start_pos;
  QPoint p_end_pos;
  QBitmap p_mask;

  std::string p_save_path;
};
DOODLE_NAMESPACE_E
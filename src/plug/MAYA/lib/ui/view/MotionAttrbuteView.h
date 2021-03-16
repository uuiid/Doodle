#pragma once

#include <MotionGlobal.h>
#include <lib/kernel/MotionFile.h>

#include <QtWidgets/qwidget.h>

class QLabel;
class QPlainTextEdit;
class QLineEdit;

namespace doodle::motion::ui {
class MotionAttrbuteView : public QWidget {
  Q_OBJECT
 private:
  kernel::MotionFilePtr p_MotionFile;
  kernel::PlayerMotionPtr p_MotionPlayer;


  QLabel* p_image;
  QLabel* p_user_label;
  QPlainTextEdit* p_info_text;
  QLineEdit* p_tiles_text;

  QMetaObject::Connection p_info_connection;
  QMetaObject::Connection p_tiles_connection;
 Q_SIGNALS:
  void doodleSetImage(const QPixmap& image);

 public:
  MotionAttrbuteView(QWidget* parent = nullptr);

  void setMotionFile(const kernel::MotionFilePtr& data);
  void doodleClear();
};

}  // namespace doodle::motion::ui
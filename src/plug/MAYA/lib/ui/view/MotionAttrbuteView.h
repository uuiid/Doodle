#pragma once

#include <MotionGlobal.h>
#include <lib/kernel/MotionFile.h>

#include <QtWidgets/qwidget.h>

class QLabel;
class QTextEdit;
class QLineEdit;

namespace doodle::motion::ui {
class MotionAttrbuteView : public QWidget {
  Q_OBJECT
 private:
  kernel::MotionFilePtr p_MotionFile;
  kernel::PlayerMotionPtr p_MotionPlayer;

  QLabel* p_image;
  QLabel* p_user_label;
  QTextEdit* p_info_text;
  QLineEdit* p_tiles_text;

 public:
  MotionAttrbuteView(QWidget* parent = nullptr);

  void setMotionFile(const kernel::MotionFilePtr& data);
};

}  // namespace doodle::motion::ui
#include <lib/ui/view/MotionAttrbuteView.h>

#include <lib/kernel/PlayerMotion.h>

#include <QtWidgets/qlayout.h>
#include <QtWidgets/qlabel.h>
#include <QtWidgets/qtextedit.h>
#include <QtWidgets/qlineedit.h>

#include <QtGui/qimage.h>
// #include <QtMultimedia/qmediaplayer.h>

namespace doodle::motion::ui {
MotionAttrbuteView::MotionAttrbuteView(QWidget* parent)
    : QWidget(parent),
      p_MotionFile(),
      p_MotionPlayer(std::make_shared<kernel::PlayerMotion>()),
      p_image(),
      p_user_label(),
      p_info_text(),
      p_tiles_text() {
  auto layout = new QGridLayout(this);

  auto k_tiles_label = new QLabel(tr("名称: "));
  auto k_user_label  = new QLabel(tr("制作人: "));
  auto k_info_label  = new QLabel(tr("信息: "));

  auto p_image      = new QLabel();
  auto p_user_label = new QLabel(tr("匿名"));
  auto p_info_text  = new QTextEdit();
  auto p_tiles_text = new QLineEdit(tr("无"));
  //显示视频
  layout->addWidget(p_image, 0, 0, 1, 2);
  //显示标题
  layout->addWidget(k_tiles_label, 1, 0, 1, 1);
  layout->addWidget(p_tiles_text, 1, 1, 1, 1);
  //显示制作人
  layout->addWidget(k_user_label, 2, 1, 1, 1);
  layout->addWidget(p_user_label, 2, 1, 1, 1);
  //显示注释
  layout->addWidget(k_info_label, 3, 0, 1, 1);
  layout->addWidget(p_info_text, 4, 0, 1, 2);

  layout->setRowStretch(0, 10);
  layout->setRowStretch(1, 1);
  layout->setRowStretch(2, 1);
  layout->setRowStretch(3, 1);
  layout->setRowStretch(4, 10);

  layout->setColumnStretch(0, 1);
  layout->setColumnStretch(1, 4);
}

void MotionAttrbuteView::setMotionFile(const kernel::MotionFilePtr& data) {
  p_MotionFile = data;
}

}  // namespace doodle::motion::ui
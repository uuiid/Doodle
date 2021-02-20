#include <lib/ui/MotionSettingWidget.h>
#include <lib/kernel/MotionSetting.h>

#include <QtWidgets/qboxlayout.h>
#include <QtWidgets/qlabel.h>
#include <QtWidgets/qlineedit.h>
#include <QtWidgets/qpushbutton.h>

namespace doodle::motion::ui {
MotionSettingWidget::MotionSettingWidget(QWidget *parent)
    : QWidget(parent) {
  auto layout = new QGridLayout(this);

  auto label_lib_root = new QLabel();
  label_lib_root->setObjectName("label_lib_root");
  auto label_user = new QLabel();
  label_user->setObjectName("label_user");

  auto lineEdit_root = new QLineEdit();
  label_lib_root->setObjectName("label_lib_root");
  auto lineEdit_user = new QLineEdit();
  lineEdit_user->setObjectName("lineEdit_user");
  auto button_open_path = new QPushButton();
  button_open_path->setObjectName("button_open_path");
  auto butten_return_up = new QPushButton();
  butten_return_up->setObjectName("butten_return_up");
  butten_return_up->setText("返回");

  label_lib_root->setText("动作库根路径");
  label_user->setText("制作人");

  button_open_path->setText("...");

  //链接返回函数
  connect(butten_return_up, &QPushButton::clicked,
          this, [=]() { this->ReturnUp(); });
  //链接设置姓名函数
  connect(lineEdit_root, &QLineEdit::textChanged,
          this, [=](const QString &str) {
            auto &set = doodle::motion::kernel::MotionSetting::Get();
            set.setMotionLibRoot(str.toStdU16String());
          });
  //链接设置根路径函数

  layout->addWidget(label_lib_root, 0, 0, 1, 1);
  layout->addWidget(label_user, 1, 0, 1, 1);
  layout->addWidget(lineEdit_root, 0, 1, 1, 1);
  layout->addWidget(lineEdit_user, 1, 1, 1, 2);
  layout->addWidget(button_open_path, 0, 2, 1, 1);
  layout->addWidget(butten_return_up, 2, 0, 1, 3);

  layout->setRowStretch(0, 1);
  layout->setRowStretch(1, 1);
  layout->setRowStretch(2, 1);

  layout->setColumnStretch(0, 1);
  layout->setColumnStretch(1, 5);
  layout->setColumnStretch(2, 1);
}
}  // namespace doodle::motion::ui
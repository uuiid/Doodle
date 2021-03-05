#include <lib/ui/MotionSettingWidget.h>
#include <lib/kernel/MotionSetting.h>

#include <maya/MGlobal.h>

#include <QtWidgets/qboxlayout.h>
#include <QtWidgets/qlabel.h>
#include <QtWidgets/qlineedit.h>
#include <QtWidgets/qpushbutton.h>
#include <QtWidgets/qfiledialog.h>

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
  butten_return_up->setText("返回并保存");

  label_lib_root->setText("动作库根路径");
  label_user->setText("制作人");

  button_open_path->setText("...");

  auto &set = kernel::MotionSetting::Get();
  lineEdit_user->setText(QString::fromStdString(set.User()));
  lineEdit_root->setText(QString::fromStdString(set.MotionLibRoot().generic_string()));

  //链接返回函数
  connect(butten_return_up, &QPushButton::clicked,
          this, [=]() {
            auto &set = doodle::motion::kernel::MotionSetting::Get();
            set.save();
            this->ReturnUp();
          });
  //链接设置根路径函数
  connect(lineEdit_root, &QLineEdit::textEdited,
          this, [=](const QString &str) {
            auto &set = doodle::motion::kernel::MotionSetting::Get();
            set.setMotionLibRoot(FSys::path{str.toStdString()});
          });
  //链接根路径按钮函数
  connect(button_open_path, &QPushButton::clicked, this,
          [=]() {
            auto &set   = doodle::motion::kernel::MotionSetting::Get();
            QString dir = QFileDialog::getExistingDirectory(
                this, tr("Open"),
                QString::fromStdString(set.MotionLibRoot().generic_string()),
                QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
            if (dir.isEmpty()) return;

            lineEdit_root->setText(dir);
            set.setMotionName("doodle_motion");
            set.setMotionLibRoot(dir.toStdString());
          });

  //链接设置姓名函数
  connect(lineEdit_user, &QLineEdit::textEdited, this,
          [=](const QString &string) {
            auto &set = doodle::motion::kernel::MotionSetting::Get();
            set.setUser(string.toStdString());
          });

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
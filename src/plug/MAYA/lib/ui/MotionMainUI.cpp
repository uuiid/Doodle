#include <lib/ui/MotionMainUI.h>

#include <lib/kernel/MotionSetting.h>

#include <maya/MGlobal.h>

#include <lib/ui/MotionSettingWidget.h>
#include <lib/ui/MotionLibWidget.h>

#include <QtWidgets/QGridLayout.h>
#include <QtWidgets/QPushButton.h>

namespace doodle::motion::ui {

MotionMainUI::MotionMainUI(QWidget *parent, Qt::WindowFlags flags)
    : QMainWindow(parent, flags),
      p_layout(),
      p_centralWidget(),
      p_setting_widget() {
  resize(1200, 800);

  setWindowTitle("doodle动作库");

  //设置中央小部件
  p_centralWidget = new QWidget(this);
  p_centralWidget->setObjectName("doodleMotionCentralWidght");
  setCentralWidget(p_centralWidget);
  //创建布局
  p_layout = new QGridLayout(p_centralWidget);

  //创建设置面板
  p_setting_widget = new doodle::motion::ui::MotionSettingWidget();
  p_layout->addWidget(p_setting_widget);

  auto rootPath = doodle::motion::kernel::MotionSetting::Get().MotionLibRoot();
  if (rootPath.empty()) {
    setMotionLib();
  } else {
    openMotionLib();
  }
}

void MotionMainUI::setMotionLib() {
  resize(1200, 100);

  //断开上次的所有链接
  disconnect(p_setting_widget,
             &doodle::motion::ui::MotionSettingWidget::ReturnUp,
             nullptr, nullptr);
  //链接返回代码
  connect(p_setting_widget,
          &doodle::motion::ui::MotionSettingWidget::ReturnUp,
          this,
          &doodle::motion::ui::MotionMainUI::openMotionLib);
}

void MotionMainUI::openMotionLib() {
  p_setting_widget->hide();
}

void MotionMainUI::createMainWidget() {
  auto k_motion = new MotionLibWidget();
  p_layout->addWidget(k_motion);
  k_motion->show();
}

}  // namespace doodle::motion::ui
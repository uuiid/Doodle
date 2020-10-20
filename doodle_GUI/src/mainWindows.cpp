#include "mainWindows.h"

#include "ProjectWidget.h"

#include "settingWidget.h"
#include "systemTray.h"


DOODLE_NAMESPACE_S

mainWindows::mainWindows(QWidget *parent)
    : QMainWindow(parent),
      exitAction(nullptr),
      refreshAction(nullptr),
      openSetWindows(nullptr),
      p_menu_bar_(nullptr),
      p_menu_(nullptr),
      p_status_bar_(nullptr),
      centralWidget(nullptr),
      p_b_box_layout_(nullptr),
      p_prject_widght_(nullptr),
      p_setting_widget_(nullptr){
  doodle_init();
}

mainWindows::~mainWindows() = default;

void mainWindows::doodle_init() {
  //初始化自身
  if (objectName().isEmpty())
    setObjectName(QString::fromUtf8("mainWindows"));
  resize(1200, 800);
  setWindowTitle(tr("MainWindow"));

  //添加动作和菜单
  doodle_createAction();

  //设置中央小部件
  centralWidget = new QWidget(this);
  centralWidget->setObjectName(QString::fromUtf8("mainWindows"));
  //添加中央小部件
  setCentralWidget(centralWidget);

  //设置基本布局
  p_b_box_layout_ = new QVBoxLayout(centralWidget);
  p_b_box_layout_->setSpacing(0);
  p_b_box_layout_->setContentsMargins(0, 0, 0, 0);
  p_b_box_layout_->setObjectName(QString::fromUtf8("p_b_box_layout_"));

  //开始设置项目小部件
  auto prj = new QListWidget(centralWidget);
  prj->setObjectName("prj");
  prj->addItems(doCore::coreSet::getCoreSet().getAllPrjName());
  prj->setFlow(QListView::LeftToRight);

  p_prject_widght_ = new ProjectWidget(centralWidget);
  p_prject_widght_->setObjectName("p_prject_widght_");
  p_b_box_layout_->addWidget(prj,1);
  p_b_box_layout_->addWidget(p_prject_widght_,35);

  p_setting_widget_ = new settingWidget(centralWidget);
  //连接项目更改设置
  connect(prj, &QListWidget::itemClicked,
          p_setting_widget_, [=](QListWidgetItem *item)mutable{
    p_setting_widget_->setProject(item->text());});
  connect(prj, &QListWidget::itemClicked,
          p_prject_widght_, &ProjectWidget::refresh);
}

void mainWindows::doodle_createAction() {
  //添加菜单栏
  p_menu_bar_ = new QMenuBar(this);
  p_menu_bar_->setObjectName(QString::fromUtf8("p_menu_bar_"));
  p_menu_bar_->setGeometry(QRect(0, 0, 640, 31));
  this->setMenuBar(p_menu_bar_);

  //添加菜单
  p_menu_ = new QMenu(p_menu_bar_);
  p_menu_->setObjectName(QString::fromUtf8("p_menu_"));
  p_menu_->setTitle(tr("&File"));
  p_menu_bar_->addAction(p_menu_->menuAction());

  //添加菜单动作
  refreshAction = new QAction(this);
  refreshAction->setObjectName(QString::fromUtf8("refreshAction"));
  refreshAction->setText(tr("Refresh"));
  refreshAction->setStatusTip(tr("刷新"));
  refreshAction->setToolTip(tr("Refresh"));
  p_menu_->addAction(refreshAction);

  openSetWindows = new QAction(this);
  openSetWindows->setObjectName(QString::fromUtf8("openSetWindows"));
  openSetWindows->setText(tr("Open Setting"));
  openSetWindows->setStatusTip(tr("打开设置"));
  openSetWindows->setToolTip(tr("Open Setting"));
  connect(openSetWindows,&QAction::triggered,
          this,&mainWindows::openSetting);
  p_menu_->addAction(openSetWindows);

  exitAction = new QAction(this);
  exitAction->setObjectName(QString::fromUtf8("exitAction"));
  exitAction->setText(tr("Exit"));
  exitAction->setStatusTip(tr("退出"));
  exitAction->setToolTip(tr("Exit"));
  p_menu_->addAction(exitAction);

  //添加状态栏
  p_status_bar_ = new QStatusBar(this);
  p_status_bar_->setObjectName(QString::fromUtf8("p_status_bar_"));
  setStatusBar(p_status_bar_);
}
void mainWindows::openSetting() {
  p_setting_widget_->setInit();
  p_setting_widget_->show();
}
DOODLE_NAMESPACE_E

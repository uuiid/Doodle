#include "mainWindows.h"

#include "src/episodes.h"
#include "src/shot.h"
#include "src/fileclass.h"
#include "src/filetype.h"

#include "episodesListWidget.h"
#include "shotListWidget.h"
#include "fileClassShotWidget.h"
#include "fileTypeShotWidget.h"
#include "shotTableWidget.h"


#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>

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
      p_episodes_list_widget_(nullptr),
      p_shot_lsit_widget_(nullptr),
      p_file_class_shot_widget_(nullptr),
      p_file_type_shot_widget_(nullptr),
      p_shot_table_widget_(nullptr){
  doodle_init();
}

mainWindows::~mainWindows() = default;

void mainWindows::doodle_init() {
  //初始化自身
  if (objectName().isEmpty())
    setObjectName(QString::fromUtf8("mainWindows"));
  resize(640, 480);
  setWindowTitle(tr("MainWindow"));

  //添加动作和菜单
  doodle_createAction();

  //设置中央小部件
  centralWidget = new QWidget(this);
  centralWidget->setObjectName(QString::fromUtf8("mainWindows"));
  //添加中央小部件
  setCentralWidget(centralWidget);

  //设置基本布局
  p_b_box_layout_ = new QHBoxLayout(centralWidget);
  p_b_box_layout_->setSpacing(0);
  p_b_box_layout_->setContentsMargins(0, 0, 0, 0);
  p_b_box_layout_->setObjectName(QString::fromUtf8("p_b_box_layout_"));

  //创建集数小部件
  p_episodes_list_widget_ = new episodesListWidget(centralWidget);
  p_episodes_list_widget_->setObjectName(QString::fromUtf8("p_episodes_list_widget_"));

  //添加镜头小部件
  p_shot_lsit_widget_ = new shotLsitWidget(centralWidget);
  p_shot_lsit_widget_->setObjectName(QString::fromUtf8("p_shot_lsit_widget_"));
  //连接集数和镜头的更新
  connect(p_episodes_list_widget_, &episodesListWidget::episodesEmit,
          p_shot_lsit_widget_, &shotLsitWidget::init);


  //添加部门小部件
  p_file_class_shot_widget_ = new fileClassShotWidget(centralWidget);
  p_file_class_shot_widget_->setObjectName(QString::fromUtf8("p_file_class_shot_widget_"));
  connect(p_shot_lsit_widget_, &shotLsitWidget::shotEmit,
          p_file_class_shot_widget_, &fileClassShotWidget::init);
  //连接刷新函数
  connect(p_episodes_list_widget_,&episodesListWidget::episodesEmit,
          p_file_class_shot_widget_,&fileClassShotWidget::clear);

  //添加种类小部件
  p_file_type_shot_widget_ = new fileTypeShotWidget(centralWidget);
  p_file_type_shot_widget_->setObjectName("p_file_type_shot_widget_");
  connect(p_file_class_shot_widget_, &fileClassShotWidget::fileClassShotEmitted,
          p_file_type_shot_widget_, &fileTypeShotWidget::init);
  connect(p_episodes_list_widget_,&episodesListWidget::episodesEmit,
          p_file_type_shot_widget_,&fileTypeShotWidget::clear);
  connect(p_shot_lsit_widget_,&shotLsitWidget::shotEmit,
          p_file_type_shot_widget_,&fileTypeShotWidget::clear);

  p_shot_table_widget_ = new shotTableWidget(centralWidget);
  p_shot_table_widget_->setObjectName("p_shot_table_widget_");
  connect(p_file_type_shot_widget_,&fileTypeShotWidget::typeEmit,
          p_shot_table_widget_, &shotTableWidget::init);
  connect(p_episodes_list_widget_,&episodesListWidget::episodesEmit,
          p_shot_table_widget_, &shotTableWidget::clear);
  connect(p_file_class_shot_widget_,&fileClassShotWidget::fileClassShotEmitted,
          p_shot_table_widget_, &shotTableWidget::clear);
  connect(p_file_type_shot_widget_,&fileTypeShotWidget::typeEmit,
          p_shot_table_widget_, &shotTableWidget::clear);

  //将小部件添加到布局中
  p_b_box_layout_->addWidget(p_episodes_list_widget_);
  p_b_box_layout_->addWidget(p_shot_lsit_widget_);
  p_b_box_layout_->addWidget(p_file_class_shot_widget_);
  p_b_box_layout_->addWidget(p_file_type_shot_widget_);
  p_b_box_layout_->addWidget(p_shot_table_widget_);
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

DOODLE_NAMESPACE_E

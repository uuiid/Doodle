#include "mainWindows.h"

#include "episodesListWidget.h"

#include <QAction>
#include <QVBoxLayout>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>


DOODLE_NAMESPACE_S

mainWindows::mainWindows(QWidget *parent) : QMainWindow(parent)
{
    doodle_init();
}

mainWindows::~mainWindows()
{
}

void mainWindows::doodle_init()
{
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
    p_b_vboxLayout = new QVBoxLayout(centralWidget);
    p_b_vboxLayout->setSpacing(0);
    p_b_vboxLayout->setContentsMargins(0, 0, 0, 0);
    p_b_vboxLayout->setObjectName(QString::fromUtf8("p_b_vboxLayout"));

    //创建集数小部件
    p_d_episodesListWidget = new episodesListWidget(centralWidget);
    p_d_episodesListWidget->setObjectName(QString::fromUtf8("p_d_episodesListWidget"));

    //将集数小部件添加到布局中
    p_b_vboxLayout->addWidget(p_d_episodesListWidget);
}

void mainWindows::doodle_createAction()
{
    //添加菜单栏
    p_d_MenuBar = new QMenuBar(this);
    p_d_MenuBar->setObjectName(QString::fromUtf8("p_d_MenuBar"));
    p_d_MenuBar->setGeometry(QRect(0, 0, 640, 31));
    this->setMenuBar(p_d_MenuBar);

    //添加菜单
    p_d_Menu = new QMenu(p_d_MenuBar);
    p_d_Menu->setObjectName(QString::fromUtf8("p_d_Menu"));
    p_d_Menu->setTitle(tr("&File"));
    p_d_MenuBar->addAction(p_d_Menu->menuAction());

    //添加菜单动作
    refreshAction = new QAction(this);
    refreshAction->setObjectName(QString::fromUtf8("refreshAction"));
    refreshAction->setText(tr("Refresh"));
    p_d_Menu->addAction(refreshAction);

    openSetWindows = new QAction(this);
    openSetWindows->setObjectName(QString::fromUtf8("openSetWindows"));
    openSetWindows->setText(tr("Open Setting"));
    p_d_Menu->addAction(openSetWindows);

    exitAction = new QAction(this);
    exitAction->setObjectName(QString::fromUtf8("exitAction"));
    exitAction->setText(tr("Exit"));
    p_d_Menu->addAction(exitAction);

    //添加状态栏
    p_d_StatusBar = new QStatusBar(this);
    p_d_StatusBar->setObjectName(QString::fromUtf8("p_d_StatusBar"));
    setStatusBar(p_d_StatusBar);
}

DOODLE_NAMESPACE_E

/*
 * @Author: your name
 * @Date: 2020-09-29 17:22:20
 * @LastEditTime: 2020-10-09 11:52:56
 * @LastEditors: your name
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_GUI\src\mainWindows.h
 */
#pragma once

#include "doodle_global.h"

#include <QMainWindow>


DOODLE_NAMESPACE_S

class mainWindows : public QMainWindow
{
private:
public:
    mainWindows(QWidget *parent = nullptr);
    ~mainWindows();

private:
    void doodle_init();
    void doodle_createAction();

private:
    QAction *exitAction;     // 退出软件
    QAction *refreshAction;  // 刷新函数
    QAction *openSetWindows; // 打开设置

    QMenuBar *p_d_MenuBar;  //菜单栏
    QMenu *p_d_Menu;  //文件菜单
    QStatusBar *p_d_StatusBar;

    QWidget *centralWidget; //中心小部件
    QVBoxLayout *p_b_vboxLayout; //布局

    episodesListWidget *p_d_episodesListWidget; //集数小部件
};

DOODLE_NAMESPACE_E
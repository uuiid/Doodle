/*
 * @Author: your name
 * @Date: 2020-09-29 17:22:20
 * @LastEditTime: 2020-10-09 16:23:08
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_GUI\src\mainWindows.h
 */
#pragma once

#include "doodle_global.h"

#include <QMainWindow>

DOODLE_NAMESPACE_S

class mainWindows : public QMainWindow {
 Q_OBJECT
 public:
  explicit mainWindows(QWidget *parent = nullptr);
  //~mainWindows() override;
  Q_DISABLE_COPY(mainWindows);

 private:
  void doodle_init();
  void doodle_createAction();

 public slots:
  void openSetting();

 private:
  QAction *exitAction;     // 退出软件
  QAction *refreshAction;  // 刷新函数
  QAction *openSetWindows; // 打开设置

  QMenuBar *p_menu_bar_;  //菜单栏
  QMenu *p_menu_;  //文件菜单
  QStatusBar *p_status_bar_;

  QWidget *centralWidget; //中心小部件
  QVBoxLayout *p_b_box_layout_; //布局

  settingWidget * p_setting_widget_;
};

DOODLE_NAMESPACE_E
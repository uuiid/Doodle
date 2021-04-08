/*
 * @Author: your name
 * @Date: 2020-09-29 17:22:20
 * @LastEditTime: 2020-10-09 16:23:08
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_GUI\src\mainWindows.h
 */
#pragma once


#include <doodle_GUI/doodle_global.h>
#include <QMainWindow>
class QListWidget;
DOODLE_NAMESPACE_S

class mainWindows : public QMainWindow {
  Q_OBJECT

 Q_SIGNALS:
  void setProgress(int value);

 public:
  explicit mainWindows(QWidget *parent = nullptr);
  //~mainWindows() override;
  Q_DISABLE_COPY(mainWindows);

 private:
  void doodle_init();
  void doodle_createAction();

 public Q_SLOTS:
  void openSetting();
  void setProject();

 private:
  QAction *exitAction;      // 退出软件
  QAction *refreshAction;   // 刷新函数
  QAction *openSetWindows;  // 打开设置

  QMenuBar *p_menu_bar_;  //菜单栏
  QMenu *p_menu_;         //文件菜单
  QStatusBar *p_status_bar_;

  QWidget *centralWidget;        //中心小部件
  QGridLayout *p_layout;  //布局

  SettingWidget *p_setting_widget_;
  QListWidget *p_project_list;
  //  assWidght * p_ass_widght_;
  //  shotWidget * p_shot_widget_;
};

DOODLE_NAMESPACE_E
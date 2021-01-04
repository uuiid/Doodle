/*
 * @Author: your name
 * @Date: 2020-10-10 10:25:56
 * @LastEditTime: 2020-10-10 14:36:12
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_GUI\src\shotClassWidget.h
 */
#pragma once

#include <doodle_global.h>
#include <core_global.h>

#include <QListView>
#include <QAbstractListModel>
#include <QStyledItemDelegate>

DOODLE_NAMESPACE_S

class shotClassWidget : public QListView {
  Q_OBJECT

 public:
  explicit shotClassWidget(QWidget *parent = nullptr);

 public:
  void clear();
 Q_SIGNALS:
  void doodleUseFilter(const filterState &state);

 private Q_SLOTS:
  //添加fileclass
  void insertFileClass();
  //私有化fileclass发射
  void _doodle_fileclass_emit(const QModelIndex &index);

 protected:
  void mousePressEvent(QMouseEvent *event) override;
  //void contextMenuEvent(QContextMenuEvent *event) override;

 private:
  shotClassModel *p_model_;
  //上下文菜单
  QMenu *p_fileClass_menu;
};

DOODLE_NAMESPACE_E
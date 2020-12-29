/*
 * @Author: your name
 * @Date: 2020-11-16 19:06:12
 * @LastEditTime: 2020-11-27 16:55:03
 * @LastEditors: your name
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_GUI\src\shotTableWidget.h
 */
//
// Created by teXiao on 2020/10/13.
//

#pragma once

#include "doodle_global.h"

#include "core_global.h"

#include <QTableView>
#include <src/fileArchive/movieArchive.h>

class QProgressDialog;
DOODLE_NAMESPACE_S

class shotTableWidget : public QTableView {
  Q_OBJECT
 public:
  explicit shotTableWidget(QWidget *parent = nullptr);

  void setModel(QAbstractItemModel *model) override;

 Q_SIGNALS:
  void useFilter(const filterState &state);

 private Q_SLOTS:

  void getSelectPath();
  void getSelectDir();
  void exportFbx();

  void doClickedSlots(const QModelIndex &index);
  void doDubledSlots(const QModelIndex &index);

  static void submitMayaFile(shotInfoPtr &info_ptr,
                             const QString &path);
  static void submitFBFile(shotInfoPtr &info_ptr, const QString &path);

  void deleteShot();

 protected:
  void contextMenuEvent(QContextMenuEvent *event) override;

  void dragEnterEvent(QDragEnterEvent *event) override;
  void dragLeaveEvent(QDragLeaveEvent *event) override;
  void dragMoveEvent(QDragMoveEvent *event) override;
  void dropEvent(QDropEvent *event) override;

 private:
  shotTypePtr p_type_ptr_;
  shotTableModel *p_model_;

  QMenu *p_menu_;

 private:
  void init();
  void insertShot(const QString &path);
  void enableBorder(const bool &isEnable);
};

DOODLE_NAMESPACE_E

/*
 * @Author: your name
 * @Date: 2020-10-19 13:26:31
 * @LastEditTime: 2020-11-30 13:23:52
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_GUI\src\assClassWidget.h
 */
//
// Created by teXiao on 2020/10/16.
//

#pragma once
#include <QListView>
#include <QStyledItemDelegate>

#include "core_global.h"
#include "doodle_global.h"

DOODLE_NAMESPACE_S
class assClassDelegate : public QStyledItemDelegate {
  Q_OBJECT
 public:
  explicit assClassDelegate(QObject* parent = nullptr);

  //创建一个提供编辑的小部件
  QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                        const QModelIndex& index) const override;

  void setEditorData(QWidget* editor, const QModelIndex& index) const override;
  void setModelData(QWidget* editor, QAbstractItemModel* model,
                    const QModelIndex& index) const override;
  void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option,
                            const QModelIndex& index) const override;
};

class assClassWidget : public QListView {
  Q_OBJECT
 public:
  explicit assClassWidget(QWidget* parent = nullptr);

  void setModel(QAbstractItemModel* model) override;
 signals:
  void initEmited();

 private:
  QMenu* p_menu_;

 private slots:
  void insertAss();
  void editAssName();
  void deleteSQLFile();

  void _doodle_ass_emit(const QModelIndex& index);

 protected:
  void contextMenuEvent(QContextMenuEvent* event) override;
};

DOODLE_NAMESPACE_E
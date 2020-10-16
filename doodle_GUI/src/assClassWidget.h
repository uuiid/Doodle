//
// Created by teXiao on 2020/10/16.
//

#pragma once
#include "doodle_global.h"
#include "core_global.h"

#include <QStyledItemDelegate>
#include <QListView>

DOODLE_NAMESPACE_S

class assClassWidget : public QListView {
 Q_OBJECT
 public:
  explicit assClassWidget(QWidget *parent = nullptr);

  void setModel(QAbstractItemModel *model) override;
 public slots:
  void init(const doCore::fileClassPtr &file_class_ptr);
  void clear();
 signals:
  void fileClassEmited(const doCore::fileClassPtr &ptr);

 private:
  //模型指针
  assClassModel *p_model_;

  QMenu *p_menu_;

  doCore::fileClassPtr p_class_ptr_;

 private slots:
  void insertAss();
  void editAssName();
  void _doodle_ass_emit(const QModelIndex &index);

 protected:
  void contextMenuEvent(QContextMenuEvent *event) override;
};

DOODLE_NAMESPACE_E
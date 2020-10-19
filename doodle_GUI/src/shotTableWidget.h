//
// Created by teXiao on 2020/10/13.
//

#pragma once

#include "doodle_global.h"

#include "core_global.h"

#include <QTableView>

DOODLE_NAMESPACE_S

class shotTableWidget : public QTableView {
 Q_OBJECT
 public:
  explicit shotTableWidget(QWidget *parent = nullptr);

  void setModel(QAbstractItemModel *model) override;
 public slots:
  void init(const doCore::fileTypePtr &file_type_ptr);
  void clear();
 private slots:
  void insertShot();

 protected:
  void contextMenuEvent(QContextMenuEvent *event) override;

 private:
  doCore::fileTypePtr p_type_ptr_;
  shotTableModel *p_model_;

  QMenu *p_menu_;
};

DOODLE_NAMESPACE_E



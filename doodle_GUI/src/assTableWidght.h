//
// Created by teXiao on 2020/10/17.
//

#pragma once

#include "doodle_global.h"
#include "core_global.h"

#include <QTableView>

DOODLE_NAMESPACE_S
class assTableWidght : public QTableView {
 Q_OBJECT
 public:
  explicit assTableWidght(QWidget *parent = nullptr);

  void setModel(QAbstractItemModel *model) override;
 public:
  void init(const doCore::shotTypePtr &file_type_ptr);
  void clear();

 private:
  doCore::assInfoPtrList p_info_ptr_list_;
  assTableModel *p_model_;

  QMenu *p_menu_;
  doCore::shotTypePtr p_file_type_ptr_;

 private slots:
  void insertAss();

 protected:
  void contextMenuEvent(QContextMenuEvent *event) override;
};

DOODLE_NAMESPACE_E

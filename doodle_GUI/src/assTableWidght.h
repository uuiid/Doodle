//
// Created by teXiao on 2020/10/17.
//

#pragma once

#include "doodle_global.h"
#include "core_global.h"

#include <QTableView>
#include <future>
class QProgressDialog;
DOODLE_NAMESPACE_S
class assTableWidght : public QTableView {
 Q_OBJECT
 public:
  explicit assTableWidght(QWidget *parent = nullptr);

  void setModel(QAbstractItemModel *model) override;
 public:

 private:
  void init();

 private:
  assTableModel *p_model_;
  QMenu *p_menu_;
 private:
  void insertAss(const QString &path);
 private slots:
  void openFileDialog();

 protected:
  void contextMenuEvent(QContextMenuEvent *event) override;
};

DOODLE_NAMESPACE_E

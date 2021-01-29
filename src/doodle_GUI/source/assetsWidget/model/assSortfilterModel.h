/*
 * @Author: your name
 * @Date: 2020-11-18 10:50:13
 * @LastEditTime: 2020-11-30 13:49:23
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_GUI\src\assetsWidget\model\assSortfilterModel.h
 */
#pragma once
#include <core_global.h>
#include <doodle_global.h>
#include <QtCore/qsortfilterproxymodel.h>
DOODLE_NAMESPACE_S
class assSortfilterModel : public QSortFilterProxyModel {
  Q_OBJECT
 public:
  explicit assSortfilterModel(QObject *parent = nullptr);

 protected:
  bool filterAcceptsRow(int source_row,
                        const QModelIndex &source_parent) const override;

 private:
};

DOODLE_NAMESPACE_E
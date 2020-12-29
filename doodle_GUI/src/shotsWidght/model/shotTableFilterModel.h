#pragma once

#include <doodle_global.h>
#include <core_global.h>

#include <QtCore/QSortFilterProxyModel>

DOODLE_NAMESPACE_S
class shotTableFilterModel : public QSortFilterProxyModel {
 public:
  shotTableFilterModel(QObject *parent = nullptr);

  void useFilter(const filterState &useFilter);

 protected:
  bool filterAcceptsRow(int source_row,
                        const QModelIndex &source_parent) const override;
  bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

 private:
  filterState p_useFilter_e;
};

DOODLE_NAMESPACE_E
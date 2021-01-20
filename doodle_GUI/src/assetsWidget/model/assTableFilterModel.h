#pragma once

#include <core_global.h>
#include <doodle_global.h>

#include <QtCore/QSortFilterProxyModel>

DOODLE_NAMESPACE_S
class assTableFilterModel : public QSortFilterProxyModel {
  Q_OBJECT
 public:
  explicit assTableFilterModel(QObject *parent = nullptr);

  void useFilter(const filterState &useFilter, const assTypePtr &ass_t);

 protected:
  bool filterAcceptsRow(int source_row,
                        const QModelIndex &source_parent) const override;
  bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

 private:
  filterState p_useFilter_e;
  assTypePtr p_type;
};

DOODLE_NAMESPACE_E

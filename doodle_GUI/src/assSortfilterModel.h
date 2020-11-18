#pragma once
#include <core_global.h>
#include <doodle_global.h>
#include <QtCore/qsortfilterproxymodel.h>
DOODLE_NAMESPACE_S
class assSortfilterModel : public QSortFilterProxyModel {
 Q_OBJECT
 public:
  explicit assSortfilterModel(QObject *parent = nullptr);

  DOODLE_NODISCARD bool lessThan(const QModelIndex &lift, const QModelIndex &right) const override;
  DOODLE_NODISCARD bool filterAcceptsRow(int soureRow, const QModelIndex &sourceParent) const override;
 private:

};

DOODLE_NAMESPACE_E
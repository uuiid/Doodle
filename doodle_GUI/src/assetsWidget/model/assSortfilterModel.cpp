#include <src/assetsWidget/model/assSortfilterModel.h>
#include <core_doQt.h>
DOODLE_NAMESPACE_S
assSortfilterModel::assSortfilterModel(QObject *parent)
    : QSortFilterProxyModel(parent) {
  setSortRole(Qt::UserRole);
  setFilterRole(Qt::UserRole);

}

bool assSortfilterModel::lessThan(const QModelIndex &lift, const QModelIndex &right) const {
  auto left_core = sourceModel()->data(lift).value<doCore::assInfoPtr>();
  auto right_core = sourceModel()->data(right).value<doCore::assInfoPtr>();
  return false;
}

bool assSortfilterModel::filterAcceptsRow(int soureRow, const QModelIndex &sourceParent) const {
  return false;
}

DOODLE_NAMESPACE_E
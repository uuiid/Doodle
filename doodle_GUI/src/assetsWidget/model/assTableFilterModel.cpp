#include "assTableFilterModel.h"
#include <core_doQt.h>
DOODLE_NAMESPACE_S

assTableFilterModel::assTableFilterModel(QObject *parent)
    : QSortFilterProxyModel(parent),
      p_useFilter_e(filterState::notFilter) {
  setSortRole(Qt::UserRole);
  setFilterRole(Qt::UserRole);
  sort(0);
}

void assTableFilterModel::useFilter(const filterState &useFilter) {
  p_useFilter_e = useFilter;
  invalidate();
}

bool assTableFilterModel::filterAcceptsRow(int source_row,
                                           const QModelIndex &source_parent) const {
  auto assData = sourceModel()
                     ->index(source_row, 0)
                     .data(Qt::UserRole)
                     .value<doCore::assInfoPtr>();

  auto k_ass = doCore::assFileSqlInfo::Instances();

  auto k_item = std::find_if(k_ass.begin(), k_ass.end(), [=](doCore::assFileSqlInfo *item) -> bool {
    return item->getAssType() == assData->getAssType() &&
           item->getAssClass() == assData->getAssClass();
  });
  switch (p_useFilter_e) {
    case filterState::notFilter:
      return (*k_item) == assData.get();
    case filterState::useFilter:
      return assData->getAssType() == doCore::coreDataManager::get().getAssTypePtr();
    case filterState::showAll:
      return true;
    default:
      return true;
  }
}

bool assTableFilterModel::lessThan(const QModelIndex &left, const QModelIndex &right) const {
  auto k_left  = left.data(Qt::UserRole).value<doCore::assInfoPtr>();
  auto k_right = right.data(Qt::UserRole).value<doCore::assInfoPtr>();
  if (k_left || k_right) {
    return doCore::assFileSqlInfo::sortType(k_left, k_right);
  } else {
    return false;
  }
}

DOODLE_NAMESPACE_E
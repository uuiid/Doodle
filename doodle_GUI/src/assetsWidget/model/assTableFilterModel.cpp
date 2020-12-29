#include "assTableFilterModel.h"
#include < core_Cpp.h>
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
  auto k_Data = sourceModel()
                    ->index(source_row, 0)
                    .data(Qt::UserRole)
                    .value<assInfoPtr>();

  switch (p_useFilter_e) {
    case filterState::notFilter: {
      auto k_ass = assFileSqlInfo::Instances();

      auto k_item = std::find_if(k_ass.begin(), k_ass.end(),
                                 [=](assFileSqlInfo *item) -> bool {
                                   return item->getAssType() == k_Data->getAssType() &&
                                          item->getAssClass() == k_Data->getAssClass();
                                 });
      return (*k_item) == k_Data.get();
      break;
    }

    case filterState::useFilter:
      return k_Data->getAssType() == coreDataManager::get().getAssTypePtr();
      break;

    case filterState::showAll:
      return true;
      break;

    default:
      return true;
  }
}

bool assTableFilterModel::lessThan(const QModelIndex &left, const QModelIndex &right) const {
  auto k_left  = left.data(Qt::UserRole).value<assInfoPtr>();
  auto k_right = right.data(Qt::UserRole).value<assInfoPtr>();
  if (k_left && k_right) {
    return assFileSqlInfo::sortType(k_left, k_right);
  } else {
    return false;
  }
}

DOODLE_NAMESPACE_E
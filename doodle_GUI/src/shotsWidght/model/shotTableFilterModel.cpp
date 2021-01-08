#include "shotTableFilterModel.h"

#include < core_Cpp.h>
DOODLE_NAMESPACE_S
shotTableFilterModel::shotTableFilterModel(QObject *parent)
    : QSortFilterProxyModel(parent) {
}

void shotTableFilterModel::useFilter(const filterState &useFilter) {
  p_useFilter_e = useFilter;
  invalidate();
}

bool shotTableFilterModel::filterAcceptsRow(int source_row,
                                            const QModelIndex &source_parent) const {
  auto k_Data = sourceModel()
                    ->index(source_row, 0)
                    .data(Qt::UserRole)
                    .value<shotInfoPtr>();
  if (!k_Data) return false;

  switch (p_useFilter_e) {
    case filterState::notFilter: {
      auto k_list = shotFileSqlInfo::Instances();
      auto k_item = std::find_if(k_list.begin(), k_list.end(),
                                 [=](shotFileSqlInfo *item) -> bool {
                                   return item->getEpisdes() == k_Data->getEpisdes() &&
                                          item->getShot() == k_Data->getShot() &&
                                          item->getShotclass() == k_Data->getShotclass() &&
                                          item->getShotType() == k_Data->getShotType();
                                 });
      if ((*k_item)->isNULL()) return true;

      return (*k_item)->getIdP() == k_Data->getIdP();
      break;
    };

    case filterState::useFilter:
      return k_Data->getShotType() == coreDataManager::get().getShotTypePtr() ||
             k_Data->getShotclass() == coreDataManager::get().getShotClassPtr();
      break;

    case filterState::showAll:
      return true;
      break;

    default:
      return true;
      break;
  }
}

bool shotTableFilterModel::lessThan(const QModelIndex &left, const QModelIndex &right) const {
  auto k_left  = left.data(Qt::UserRole).value<shotInfoPtr>();
  auto k_right = right.data(Qt::UserRole).value<shotInfoPtr>();
  if (k_left && k_right) {
    return shotFileSqlInfo::sort(k_left, k_right);
  } else {
    return false;
  }
}

DOODLE_NAMESPACE_E
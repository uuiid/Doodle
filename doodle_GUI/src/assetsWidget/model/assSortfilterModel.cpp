/*
 * @Author: your name
 * @Date: 2020-11-18 10:50:54
 * @LastEditTime: 2020-11-30 14:03:38
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_GUI\src\assetsWidget\model\assSortfilterModel.cpp
 */
#include <src/assetsWidget/model/assSortfilterModel.h>
#include <core_doQt.h>
DOODLE_NAMESPACE_S
assSortfilterModel::assSortfilterModel(QObject *parent)
    : QSortFilterProxyModel(parent) {
  setSortRole(Qt::UserRole);
  setFilterRole(Qt::UserRole);
}
bool assSortfilterModel::filterAcceptsRow(
    int source_row, const QModelIndex &source_parent) const {
  auto ass_ptr = sourceModel()
                     ->index(source_row, 0)
                     .data(Qt::UserRole)
                     .value< assClassPtr>();
  if (ass_ptr) {
    auto str = ass_ptr->getAssClassQ(false);
    auto t   = str.contains(filterRegularExpression());
    return t;

    // return ass_ptr->getAssClassQ(false).contains(filterRegExp());
  } else {
    return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
  }
}

DOODLE_NAMESPACE_E
//
// Created by teXiao on 2020/10/14.
//

#include <iostream>
#include <memory>

#include <core_doQt.h>
#include "shotEpsListModel.h"

#include <boost/numeric/conversion/cast.hpp>
DOODLE_NAMESPACE_S

shotEpsListModel::shotEpsListModel(QObject *parent)
    : QAbstractListModel(parent), eplist() {
  init();
}
shotEpsListModel::~shotEpsListModel() = default;
int shotEpsListModel::rowCount(const QModelIndex &parent) const {
  return boost::numeric_cast<int>(eplist.size());
}
QVariant shotEpsListModel::data(const QModelIndex &index, int role) const {
  auto var = QVariant();
  if (!index.isValid()) return var;

  if (index.row() >= eplist.size()) return var;

  switch (role) {
    case Qt::DisplayRole:
      var = eplist[index.row()]->getEpisdes_QStr();
      break;
    case Qt::EditRole:
      var = eplist[index.row()]->getEpisdes();
      break;
    case Qt::UserRole:
      var = QVariant::fromValue(eplist[index.row()]);
      break;
    default:
      break;
  }

  return var;
}
Qt::ItemFlags shotEpsListModel::flags(const QModelIndex &index) const {
  if (!index.isValid()) return Qt::ItemIsEnabled;

  if (eplist[index.row()]->isInsert())
    return QAbstractListModel::flags(index);
  else
    return Qt::ItemIsEditable | Qt::ItemIsEnabled |
           QAbstractListModel::flags(index);
}
bool shotEpsListModel::setData(const QModelIndex &index, const QVariant &value,
                               int role) {
  init();
  if (index.isValid() && role == Qt::EditRole) {
    //确认镜头不重复和没有提交
    bool isHasEps = false;
    for (auto &&x : eplist) {
      if (value.toInt() == x->getEpisdes() && x->isInsert()) {
        isHasEps = true;
        break;
      }
    }
    if (isHasEps)
      return false;
    else {
      eplist[index.row()]->setEpisdes(value.toInt());
      eplist[index.row()]->insert();
      dataChanged(index, index, {role});
      return true;
    }
  }
  return false;
}
bool shotEpsListModel::insertRows(int position, int rows,
                                  const QModelIndex &index) {
  beginInsertRows(index, position, position + rows - 1);
  for (int row = 0; row < rows; ++row) {
    eplist.insert(eplist.begin() + position,
                  std::make_shared<doCore::episodes>());
  }
  endInsertRows();
  return true;
}
bool shotEpsListModel::removeRows(int position, int rows,
                                  const QModelIndex &index) {
  beginRemoveRows(index, position, position + rows - 1);
  for (int row = 0; row < rows; ++row) {
    eplist.erase(eplist.begin() + position);
  }
  endRemoveRows();
  return true;
}
void shotEpsListModel::init() {
  clear();
  auto tmp_eps = doCore::episodes::getAll();
  if (tmp_eps.empty()) return;
  beginInsertRows(QModelIndex(), 0,
                  boost::numeric_cast<int>(tmp_eps.size()) - 1);
  eplist = tmp_eps;
  endInsertRows();
}
void shotEpsListModel::clear() {
  if (eplist.empty()) return;
  beginResetModel();
  eplist.clear();
  endResetModel();
  doCore::coreDataManager::get().setEpisodesPtr(nullptr);
}
DOODLE_NAMESPACE_E
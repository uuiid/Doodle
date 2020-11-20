//
// Created by teXiao on 2020/10/14.
//

#include "shotListModel.h"

#include <memory>
#include <core_doQt.h>

#include "Logger.h"
#include <boost/numeric/conversion/cast.hpp>
DOODLE_NAMESPACE_S

shotListModel::shotListModel(QObject *parent)
    : QAbstractListModel(parent),
      shotlist() {
}

shotListModel::~shotListModel() = default;

int shotListModel::rowCount(const QModelIndex &parent) const {
  return boost::numeric_cast<int>(shotlist.size());
}

QVariant shotListModel::data(const QModelIndex &index, int role) const {
  auto var = QVariant();
  if (!index.isValid())
    return var;

  if (index.row() >= shotlist.size())
    return var;
  switch (role) {
    case Qt::DisplayRole:
      var = shotlist[index.row()]->getShotAndAb_strQ();
      break;
    case Qt::EditRole: {
      auto map = QMap<QString, QVariant>{{"shot", 1}, {"shotAb", ""}};
      const auto &shot = shotlist[index.row()];
      map["shot"] = shot->getShot();
      map["shotAb"] = DOTOS(shot->getShotAb_str());
      var = map;
      break;
    }
    case Qt::UserRole:
      var = QVariant::fromValue(shotlist[index.row()]);
      break;
    default:
      break;
  }


  if (role == Qt::DisplayRole || role == Qt::EditRole) {
    return shotlist[index.row()]->getShotAndAb_strQ();
  } else if (role == Qt::UserRole) {
    return QVariant::fromValue(shotlist[index.row()]);
  } else {
    return QVariant();
  }
}

QVariant shotListModel::headerData(int section,
                                   Qt::Orientation orientation,
                                   int role) const {
  if (role != Qt::DisplayRole)
    return QVariant();

  if (orientation == Qt::Horizontal)
    return QStringLiteral("Column %1").arg(section);
  else
    return QStringLiteral("Row %1").arg(section);
}

Qt::ItemFlags shotListModel::flags(const QModelIndex &index) const {
  if (!index.isValid())
    return Qt::ItemIsEnabled;

  if (shotlist[index.row()]->isInsert())
    return QAbstractListModel::flags(index);
  else
    return Qt::ItemIsEditable | Qt::ItemIsEnabled | QAbstractListModel::flags(index);
}

bool shotListModel::setData(const QModelIndex &index, const QVariant &value, int role) {
  QMap infoMap = value.toMap();
  if (index.isValid() && role == Qt::EditRole) {
    //确认镜头不重复和没有提交
    //这个函数不设置AB镜
    bool isHasShot = false;
    for (auto &&x : shotlist) {
      if ((infoMap["shot"].toInt() == x->getShot()
          && infoMap["shotAb"].toString() == x->getShotAb_strQ())
          && x->isInsert()) {
        isHasShot = true;
        break;
      }
    }

    if (isHasShot)
      return false;
    else {
      shotlist[index.row()]->setShot(infoMap["shot"].toInt(), infoMap["shotAb"].toString());
      shotlist[index.row()]->setEpisodes(doCore::coreDataManager::get().getEpisodesPtr());
      shotlist[index.row()]->insert();
      emit dataChanged(index, index, {role});
      return true;
    }
  }
  return false;
}

bool shotListModel::insertRows(int position, int rows, const QModelIndex &index) {
  beginInsertRows(QModelIndex(), position, position + rows - 1);

  for (int row = 0; row < rows; ++row) {
    shotlist.insert(shotlist.begin() + position, std::make_shared<doCore::shot>());
  }
  endInsertRows();
  return true;
}

bool shotListModel::removeRows(int position, int rows, const QModelIndex &index) {
  beginRemoveRows(index, position, position + rows - 1);
  for (int row = 0; row < rows; ++row) {
    shotlist.erase(shotlist.begin() + position);
  }
  endRemoveRows();
  return true;
}
void shotListModel::init() {
  doCore::shotPtrList tmp_shot_list = doCore::shot::getAll(doCore::coreDataManager::get().getEpisodesPtr());

  clear();

  beginInsertRows(QModelIndex(), 0, boost::numeric_cast<int>(tmp_shot_list.size()));
  shotlist = tmp_shot_list;
  endInsertRows();
}
void shotListModel::clear() {
  if (shotlist.empty()) return;
  beginResetModel();
  shotlist.clear();
  endResetModel();
  doCore::coreDataManager::get().setShotPtr(nullptr);
}

DOODLE_NAMESPACE_E

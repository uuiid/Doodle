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
      auto map         = QMap<QString, QVariant>{{"shot", 1}, {"shotAb", ""}};
      const auto &shot = shotlist[index.row()];
      map["shot"]      = shot->getShot();
      map["shotAb"]    = DOTOS(shot->getShotAb_str());
      var              = map;
      break;
    }
    case Qt::UserRole:
      var = QVariant::fromValue(shotlist[index.row()]);
      break;
    default:
      break;
  }
  return var;
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

  // if (shotlist[index.row()]->isInsert())
  //   return QAbstractListModel::flags(index);
  // else
  return Qt::ItemIsEditable | Qt::ItemIsEnabled | QAbstractListModel::flags(index);
}

bool shotListModel::setData(const QModelIndex &index, const QVariant &value, int role) {
  QMap infoMap = value.toMap();
  if (index.isValid() && role == Qt::EditRole) {
    //确认镜头不重复和没有提交
    //这个函数不设置AB镜

    auto find_shot = std::find_if(
        shotlist.begin(), shotlist.end(),
        [=](shotPtr &shot_ptr) -> bool {
          return infoMap["shot"].toInt() == shot_ptr->getShot() &&
                 infoMap["shotAb"].toString() == shot_ptr->getShotAb_strQ() &&
                 shot_ptr->isInsert();
        });

    if (find_shot == shotlist.end()) {
      shotlist[index.row()]->setShot(infoMap["shot"].toInt(), infoMap["shotAb"].toString());
      shotlist[index.row()]->updateSQL();
      dataChanged(index, index, {role});
      return true;
    }

    return false;
  }
  return false;
}

bool shotListModel::insertRows(int position, int rows, const QModelIndex &index) {
  beginInsertRows(QModelIndex(), position, position + rows - 1);

  for (int row = 0; row < rows; ++row) {
    auto k_shot = std::make_shared<shot>();
    k_shot->setEpisodes(coreDataManager::get().getEpisodesPtr());
    k_shot->insert();
    shotlist.insert(shotlist.begin() + position, k_shot);
  }
  endInsertRows();
  return true;
}

bool shotListModel::removeRows(int position, int rows, const QModelIndex &index) {
  beginRemoveRows(index, position, position + rows - 1);
  for (int row = 0; row < rows; ++row) {
    shotlist[position]->deleteSQL();
    shotlist.erase(shotlist.begin() + position);
  }
  endRemoveRows();
  return true;
}
void shotListModel::init() {
  shotPtrList tmp_shot_list = shot::getAll(coreDataManager::get().getEpisodesPtr());
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
  coreDataManager::get().setShotPtr(nullptr);
}

DOODLE_NAMESPACE_E

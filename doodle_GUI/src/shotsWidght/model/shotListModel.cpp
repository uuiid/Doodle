//
// Created by teXiao on 2020/10/14.
//

#include "shotListModel.h"

#include <memory>
#include < core_Cpp.h>

#include "Logger.h"
#include <boost/numeric/conversion/cast.hpp>
DOODLE_NAMESPACE_S

shotListModel::shotListModel(QObject *parent)
    : QAbstractListModel(parent),
      p_shot_ptr_list() {
}

shotListModel::~shotListModel() = default;

int shotListModel::rowCount(const QModelIndex &parent) const {
  return boost::numeric_cast<int>(p_shot_ptr_list.size());
}

QVariant shotListModel::data(const QModelIndex &index, int role) const {
  auto var = QVariant();
  if (!index.isValid())
    return var;
  if (index.row() >= p_shot_ptr_list.size())
    return var;

  auto k_shot = p_shot_ptr_list[index.row()];

  switch (role) {
    case Qt::DisplayRole:
      var = k_shot->getShotAndAb_strQ();
      break;
    case Qt::EditRole: {
      auto map         = QMap<QString, QVariant>{{"shot", 1}, {"shotAb", ""}};
      const auto &shot = k_shot;
      map["shot"]      = shot->getShot();
      map["shotAb"]    = DOTOS(shot->getShotAb_str());
      var              = map;
      break;
    }
    case Qt::UserRole:
      var = QVariant::fromValue(k_shot.get());
      break;
    case Qt::BackgroundColorRole:
      var = k_shot->inDeadline() ? QColor{Qt::darkGreen} : QVariant{};
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
    auto find_shot = std::find_if(
        p_shot_ptr_list.begin(), p_shot_ptr_list.end(),
        [=](shotPtr &shot_ptr) -> bool {
          return infoMap["shot"].toInt() == shot_ptr->getShot() &&
                 infoMap["shotAb"].toString() == shot_ptr->getShotAb_strQ() &&
                 shot_ptr->isInsert();
        });

    if (find_shot == p_shot_ptr_list.end()) {
      p_shot_ptr_list[index.row()]->setShot(infoMap["shot"].toInt(), infoMap["shotAb"].toString());
      p_shot_ptr_list[index.row()]->updateSQL();
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
    p_shot_ptr_list.insert(p_shot_ptr_list.begin() + position, k_shot);
  }
  endInsertRows();
  return true;
}

bool shotListModel::removeRows(int position, int rows, const QModelIndex &index) {
  beginRemoveRows(index, position, position + rows - 1);
  for (int row = 0; row < rows; ++row) {
    p_shot_ptr_list[position]->deleteSQL();
    p_shot_ptr_list.erase(p_shot_ptr_list.begin() + position);
  }
  endRemoveRows();
  return true;
}

void shotListModel::setList(const shotPtrList &list) {
  clear();
  if (list.empty()) return;
  beginInsertRows(QModelIndex(), 0, boost::numeric_cast<int>(list.size()) - 1);
  p_shot_ptr_list = list;
  endInsertRows();
}

void shotListModel::clear() {
  if (p_shot_ptr_list.empty()) return;
  beginResetModel();
  p_shot_ptr_list.clear();
  endResetModel();
  coreDataManager::get().setShotPtr(nullptr);
}

DOODLE_NAMESPACE_E

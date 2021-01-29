//
// Created by teXiao on 2020/10/14.
//

#include "shotClassModel.h"
#include < core_Cpp.h>

#include <memory>
#include <loggerlib/Logger.h>

#include <boost/numeric/conversion/cast.hpp>

DOODLE_NAMESPACE_S
shotClassModel::shotClassModel(QObject *parent)
    : QAbstractListModel(parent),
      p_shot_class_list_() {
  shotClass::insertChanged.connect(
      [this](const shotClassPtr &item) {
        doodle_dataInsert(item);
      });
}

int shotClassModel::rowCount(const QModelIndex &parent) const {
  return boost::numeric_cast<int>(p_shot_class_list_.size());
}

QVariant shotClassModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid()) return QVariant();
  if (index.row() >= p_shot_class_list_.size()) return QVariant();
  auto var = QVariant();
  switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
      var = p_shot_class_list_[index.row()]->getClass_Qstr();
      break;
    case Qt::UserRole:
      var = QVariant::fromValue(p_shot_class_list_[index.row()].get());
      break;
    default:
      break;
  }
  return var;
}

shotClassPtr shotClassModel::dataRow(const QModelIndex &index) const {
  if (!index.isValid()) return nullptr;
  if (index.row() >= p_shot_class_list_.size()) return nullptr;
  return p_shot_class_list_[index.row()];
}

QVariant shotClassModel::headerData(int section, Qt::Orientation orientation,
                                    int role) const {
  if (role != Qt::DisplayRole) return QVariant();

  if (orientation == Qt::Horizontal)
    return QStringLiteral("Column %1").arg(section);
  else
    return QStringLiteral("Row %1").arg(section);
}

Qt::ItemFlags shotClassModel::flags(const QModelIndex &index) const {
  if (!index.isValid()) return Qt::ItemIsEnabled;

  if (p_shot_class_list_[index.row()]->isInsert())
    return QAbstractListModel::flags(index);
  else
    return Qt::ItemIsEnabled | QAbstractListModel::flags(index);
}

bool shotClassModel::setData(const QModelIndex &index, const QVariant &value,
                             int role) {
  if (index.isValid() && role == Qt::EditRole) {
    //确认没有重复的fileclass
    bool isHas = false;
    for (auto &&i : p_shot_class_list_) {
      if (value.toString() == i->getClass_Qstr() || i->isInsert()) {
        isHas = true;
        break;
      }
    }

    if (isHas)
      return false;
    else {
      DOODLE_LOG_INFO("注意将fileclass提交到数据库");
      p_shot_class_list_[index.row()]->setclass(value.toString());
      dataChanged(index, index, {role});
      return true;
    }
  }
  return false;
}

bool shotClassModel::insertRows(int position, int rows,
                                const QModelIndex &index) {
  bool isHas = false;
  auto dep   = coreSet::getSet().getDepartment();
  for (auto &&i : p_shot_class_list_) {
    if (dep == i->getClass_str()) {
      isHas = true;
      break;
    }
  }
  beginInsertRows(index, position, position + rows - 1);
  if (!isHas) {
    for (int row = 0; row < rows; ++row) {
      DOODLE_LOG_INFO("插入新的fileclass镜头");
      p_shot_class_list_.insert(p_shot_class_list_.begin() + position,
                                std::make_shared<shotClass>());
      p_shot_class_list_[position]->setclass(dep);
    }
  }
  endInsertRows();
  return true;
}

bool shotClassModel::removeRows(int position, int rows,
                                const QModelIndex &index) {
  beginRemoveRows(QModelIndex(), position, position + rows - 1);

  for (int row = 0; row < rows; ++row) {
    DOODLE_LOG_INFO("去除队列中的fileclass镜头");
    p_shot_class_list_.erase(p_shot_class_list_.begin() + position);
  }

  endRemoveRows();
  return true;
}

void shotClassModel::clear() {
  if (p_shot_class_list_.empty()) return;
  beginResetModel();
  p_shot_class_list_.clear();
  endResetModel();
}

void shotClassModel::setList(const shotClassPtrList &list) {
  clear();
  if (list.empty()) return;

  beginInsertRows(QModelIndex(), 0, boost::numeric_cast<int>(list.size()) - 1);
  p_shot_class_list_ = list;
  endInsertRows();
}

void shotClassModel::doodle_dataInsert(const shotClassPtr &item) {
  auto k_size = boost::numeric_cast<int>(p_shot_class_list_.size());
  beginInsertRows(QModelIndex(), k_size, k_size);
  p_shot_class_list_.push_back(item);
  endInsertRows();
}
DOODLE_NAMESPACE_E
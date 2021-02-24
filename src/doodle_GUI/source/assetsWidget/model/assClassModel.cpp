//
// Created by teXiao on 2020/10/15.
//

#include "assClassModel.h"

#include <memory>
#include < core_Cpp.h>
#include <boost/numeric/conversion/cast.hpp>
DOODLE_NAMESPACE_S
assClassModel::assClassModel(QObject *parent)
    : QAbstractListModel(parent), p_ass_info_ptr_list_() {}
int assClassModel::rowCount(const QModelIndex &parent) const {
  return boost::numeric_cast<int>(p_ass_info_ptr_list_.size());
}
QVariant assClassModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid()) return QVariant();
  if (index.row() >= p_ass_info_ptr_list_.size()) return QVariant();

  auto var = QVariant();
  switch (role) {
    case Qt::EditRole:
    case Qt::DisplayRole:
      var = QString::fromStdString(p_ass_info_ptr_list_[index.row()]->getAssClass());
      break;
    case Qt::UserRole:
      var = QVariant::fromValue(p_ass_info_ptr_list_[index.row()].get());
    default:
      break;
  }
  return var;
}
Qt::ItemFlags assClassModel::flags(const QModelIndex &index) const {
  if (!index.isValid()) return Qt::ItemIsEditable;
  if (index.row() >= p_ass_info_ptr_list_.size()) return Qt::ItemIsEditable;

  // if (p_ass_info_ptr_list_[index.row()]->isInsert())
  //  return QAbstractListModel::flags(index);
  // else
  return Qt::ItemIsEditable | Qt::ItemIsEnabled |
         QAbstractListModel::flags(index);
}
bool assClassModel::setData(const QModelIndex &index, const QVariant &value,
                            int role) {
  if (!index.isValid()) return false;
  bool is_has = false;
  for (auto &&item : p_ass_info_ptr_list_) {
    if (value.toString().toStdString() == item->getAssClass() ||
        value.toString().toStdString() == item->getAssClass()) {
      is_has = true;
      break;
    }
  }
  auto ass = p_ass_info_ptr_list_[index.row()];
  if (!is_has) {
    if (ass->getAssClass() != value.toString().toStdString()) {
      ass->setAssClass(value.toString().toStdString());
      ass->setAssDep(coreDataManager::get().getAssDepPtr());

      Changed(index, index, {role});
      return true;
    }
  }
  return false;
}
bool assClassModel::insertRows(int position, int rows,
                               const QModelIndex &index) {
  beginInsertRows(QModelIndex(), position, position + rows - 1);
  for (int row = 0; row < rows; ++row) {
    p_ass_info_ptr_list_.insert(p_ass_info_ptr_list_.begin() + position,
                                std::make_shared<assClass>());
    p_ass_info_ptr_list_[position]->setAssDep(
        coreDataManager::get().getAssDepPtr());
  }
  endInsertRows();
  return true;
}
bool assClassModel::removeRows(int position, int rows,
                               const QModelIndex &index) {
  beginRemoveRows(QModelIndex(), position, position + rows - 1);
  for (int row = 0; row < rows; ++row) {
    auto assClass = p_ass_info_ptr_list_[position];
    if (assClass) {
      p_ass_info_ptr_list_.erase(p_ass_info_ptr_list_.begin() + position);
    }
  }
  endRemoveRows();
  return true;
}

void assClassModel::clear() {
  if (p_ass_info_ptr_list_.empty()) return;
  beginResetModel();
  p_ass_info_ptr_list_.clear();
  endResetModel();
  coreDataManager::get().setAssClassPtr(nullptr);
}

void assClassModel::setList(const assClassPtrList &setList) {
  clear();

  if (!setList.empty()) {
    beginInsertRows(QModelIndex(), 0, boost::numeric_cast<int>(setList.size()) - 1);
    p_ass_info_ptr_list_ = setList;
    endInsertRows();
  }
}
assClassModel::~assClassModel() = default;

DOODLE_NAMESPACE_E
//
// Created by teXiao on 2020/10/15.
//

#include "assClassModel.h"

#include <memory>
#include <core_doQt.h>

DOODLE_NAMESPACE_S
assClassModel::assClassModel(QObject *parent)
    : QAbstractListModel(parent),
      p_ass_info_ptr_list_() {
}
int assClassModel::rowCount(const QModelIndex &parent) const {
  return p_ass_info_ptr_list_.size();
}
QVariant assClassModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid()) return QVariant();
  if (index.row() >= p_ass_info_ptr_list_.size()) return QVariant();

  if (role == Qt::DisplayRole || role == Qt::EditRole) {
    return DOTOS(p_ass_info_ptr_list_[index.row()]->getAssClass(true));
  } else if (role == Qt::UserRole) {
    return QVariant::fromValue(p_ass_info_ptr_list_[index.row()]);
  } else {
    return QVariant();
  }
}
Qt::ItemFlags assClassModel::flags(const QModelIndex &index) const {
  if (!index.isValid()) return Qt::ItemIsEditable;
  if (index.row() >= p_ass_info_ptr_list_.size()) return Qt::ItemIsEditable;

  if (p_ass_info_ptr_list_[index.row()]->isInsert())
    return QAbstractListModel::flags(index);
  else
    return Qt::ItemIsEditable | Qt::ItemIsEnabled | QAbstractListModel::flags(index);
}
bool assClassModel::setData(const QModelIndex &index, const QVariant &value, int role) {
  if (!index.isValid()) return false;
  bool is_has = false;
  for (auto &&item : p_ass_info_ptr_list_) {
    if (value.toString().toStdString() == item->getAssClass(true)
        || value.toString().toStdString() == item->getAssClass()) {
      is_has = true;
      break;
    }
  }

  if (!is_has) {
    p_ass_info_ptr_list_[index.row()]->setAssClass(value.toString().toStdString());
    p_ass_info_ptr_list_[index.row()]->setAssDep(doCore::coreDataManager::get().getAssDepPtr());
    p_ass_info_ptr_list_[index.row()]->insert();
    emit dataChanged(index, index, {role});
    return true;
  }
  return false;
}
bool assClassModel::insertRows(int position, int rows, const QModelIndex &index) {
  beginInsertRows(QModelIndex(), position, position + rows - 1);
  for (int row = 0; row < rows; ++row) {
    p_ass_info_ptr_list_.insert(p_ass_info_ptr_list_.begin() + position,
                                std::make_shared<doCore::assClass>());
  }
  endInsertRows();
  return true;
}
bool assClassModel::removeRows(int position, int rows, const QModelIndex &index) {
  beginRemoveRows(QModelIndex(), position, position + rows - 1);
  for (int row = 0; row < rows; ++row) {
    p_ass_info_ptr_list_.erase(p_ass_info_ptr_list_.begin() + position);
  }
  endRemoveRows();
  return true;
}
void assClassModel::init(const doCore::assDepPtr &file_class_ptr) {
  if (!file_class_ptr) return;

  clear();
  auto tmp_list = doCore::assClass::getAll(file_class_ptr);
    doCore::coreDataManager::get().setAssDepPtr(file_class_ptr);
  if (!tmp_list.empty()) {
    beginInsertRows(QModelIndex(), 0, tmp_list.size() - 1);
    p_ass_info_ptr_list_ = tmp_list;
    endInsertRows();
  }
}
void assClassModel::clear() {
  if (p_ass_info_ptr_list_.empty()) return;
  beginResetModel();
  p_ass_info_ptr_list_.clear();
  endResetModel();
}
assClassModel::~assClassModel() = default;

DOODLE_NAMESPACE_E
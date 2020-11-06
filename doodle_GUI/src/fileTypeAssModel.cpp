//
// Created by teXiao on 2020/10/15.
//

#include "fileTypeAssModel.h"
#include "src/filetype.h"

DOODLE_NAMESPACE_S
fileTypeAssModel::fileTypeAssModel(QObject *parent)
    : QAbstractListModel(parent),
      p_file_type_ptr_list_(),
      p_ass_ptr_(nullptr) {}
int fileTypeAssModel::rowCount(const QModelIndex &parent) const {
  return p_file_type_ptr_list_.size();
}
QVariant fileTypeAssModel::data(const QModelIndex &index, int role) const {
  auto var = QVariant();
  if (!index.isValid()) return var;
  if (index.row() >= p_file_type_ptr_list_.size()) return var;

  if (role == Qt::DisplayRole || role == Qt::EditRole) {
    var = p_file_type_ptr_list_[index.row()]->getFileType();
  } else if (role == Qt::UserRole) {
    var = QVariant::fromValue(p_file_type_ptr_list_[index.row()]);
  }

  return var;
}
Qt::ItemFlags fileTypeAssModel::flags(const QModelIndex &index) const {
  if (!index.isValid()) return Qt::ItemIsEditable;
  if (index.row() >= p_file_type_ptr_list_.size()) return Qt::ItemIsEditable;

  if (p_file_type_ptr_list_[index.row()]->isInsert())
    return QAbstractListModel::flags(index);
  else
    return Qt::ItemIsEditable | Qt::ItemIsEnabled | QAbstractListModel::flags(index);
}
bool fileTypeAssModel::setData(const QModelIndex &index, const QVariant &value, int role) {
  if (!index.isValid()) return false;
  bool is_has = false;
  for (auto &&item : p_file_type_ptr_list_) {
    if (value.toString() == item->getAssClass()) {
      is_has = true;
      break;
    }
  }

  if (!is_has) {
    p_file_type_ptr_list_[index.row()]->setFileType(value.toString());
    p_file_type_ptr_list_[index.row()]->setAssType(p_ass_ptr_);
    p_file_type_ptr_list_[index.row()]->insert();
    emit dataChanged(index, index, {role});
    return true;
  }
  return false;

}
bool fileTypeAssModel::insertRows(int position, int rows, const QModelIndex &index) {
  beginInsertRows(QModelIndex(), position, position + rows - 1);
  for (int row = 0; row < rows; ++row) {
    p_file_type_ptr_list_.insert(position,doCore::fileTypePtr(new doCore::fileType));
  }
  endInsertRows();
  return true;
}
bool fileTypeAssModel::removeRows(int position, int rows, const QModelIndex &index) {
  beginRemoveRows(QModelIndex(),position, position + rows -1);
  for (int row = 0; row < rows; ++row) {
    p_file_type_ptr_list_.remove(position);
  }
  endRemoveRows();
  return false;
}
void fileTypeAssModel::init(const doCore::assClassPtr &ass_type_ptr) {
  clear();
  p_ass_ptr_ = ass_type_ptr;
  auto tmp_list = doCore::fileType::getAll(ass_type_ptr);
  beginInsertRows(QModelIndex(),0,tmp_list.size() -1);
  p_file_type_ptr_list_ = tmp_list;
  endInsertRows();
}
void fileTypeAssModel::clear() {
  if(!p_file_type_ptr_list_.isEmpty()) return;
  p_ass_ptr_ = nullptr;
  beginResetModel();
  p_file_type_ptr_list_.clear();
  endResetModel();
}
doCore::assClassPtr fileTypeAssModel::getAssTypePtr() const  {
  return p_ass_ptr_;
}
fileTypeAssModel::~fileTypeAssModel() = default;

DOODLE_NAMESPACE_E
//
// Created by teXiao on 2020/10/14.
//
#include "src/fileclass.h"
#include "fileClassAssModel.h"
DOODLE_NAMESPACE_S
fileClassAssModel::fileClassAssModel(QObject *parent)
:QAbstractListModel(parent)
,p_class_ptr_list_(){
  init();
}
int fileClassAssModel::rowCount(const QModelIndex &parent) const {
  return p_class_ptr_list_.size();
}
QVariant fileClassAssModel::data(const QModelIndex &index, int role) const {
  if(!index.isValid()) return QVariant();
  if(index.row() >= p_class_ptr_list_.size()) return QVariant();

  if(role == Qt::DisplayRole || role == Qt::EditRole){
    return p_class_ptr_list_[index.row()]->getFileclass_str();
  } else if (role == Qt::UserRole){
    return QVariant::fromValue(p_class_ptr_list_[index.row()]);
  } else{
    return QVariant();
  }
}
void fileClassAssModel::init() {
  clear();
  auto tmp_List  = doCore::fileClass::getAll();
  beginInsertRows(QModelIndex(),0,tmp_List.size()-1);
  p_class_ptr_list_ = tmp_List;
  endInsertRows();
}
void fileClassAssModel::clear() {
  if(!p_class_ptr_list_.isEmpty()) {
    beginResetModel();
    p_class_ptr_list_.clear();
    endResetModel();
  }
};
fileClassAssModel::~fileClassAssModel() = default;


DOODLE_NAMESPACE_E
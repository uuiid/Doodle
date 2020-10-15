//
// Created by teXiao on 2020/10/15.
//

#include "assTableModel.h"
#include "src/assfilesqlinfo.h"
#include <QJsonArray>
DOODLE_NAMESPACE_S
assTableModel::assTableModel(QObject *parent)
    : QAbstractTableModel(parent),
      p_ass_info_ptr_list_(),
      p_file_type_ptr_(nullptr) {}
int assTableModel::rowCount(const QModelIndex &parent) const {
  return p_ass_info_ptr_list_.size();
}
int assTableModel::columnCount(const QModelIndex &parent) const {
  return 5;
}
QVariant assTableModel::data(const QModelIndex &index, int role) const {
  auto var = QVariant();
  if (!index.isValid()) return var;
  if (index.row() >= p_ass_info_ptr_list_.size()) return var;

  auto ass = p_ass_info_ptr_list_[index.row()];
  if (role == Qt::DisplayRole || role == Qt::EditRole) {
    switch (index.column()) {
      case 0:
        var = QString("v%1").arg(ass->getVersionP(),
                                      4, 10, QLatin1Char('0'));
        break;
      case 1:var = ass->getInfoP().last().toString();
        break;
      case 2:var = ass->getSuffixes();
        break;
      case 3:var = ass->getUserP();
        break;
      case 4:var = ass->getIdP();
      default:var = "";
        break;
    }
  } else {
    var = QVariant::fromValue(ass);
  }
  return var;
}
QVariant assTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
  QString str;
  if (orientation == Qt::Horizontal) {
    switch (section) {
      case 0:str = tr("版本");
        break;
      case 1:str = tr("信息");
        break;
      case 2:str = tr("制作人");
        break;
      case 3:str = tr("后缀");
        break;
      case 4:str = QString("id");
        break;
      default:str = "";
        break;
    }
  } else
    str = QString(section);
  return str;
}
bool assTableModel::setData(const QModelIndex &index, const QVariant &value, int role) {
  return QAbstractItemModel::setData(index, value, role);
}
Qt::ItemFlags assTableModel::flags(const QModelIndex &index) const {
  return QAbstractTableModel::flags(index);
}
bool assTableModel::insertRows(int position, int rows, const QModelIndex &parent) {
  return QAbstractItemModel::insertRows(position, rows, parent);
}
void assTableModel::init(const doCore::fileTypePtr &file_type_ptr) {

}
void assTableModel::clear() {

}

DOODLE_NAMESPACE_E
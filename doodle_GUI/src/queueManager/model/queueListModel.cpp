#include "queueListModel.h"
#include <boost/numeric/conversion/cast.hpp>
DOODLE_NAMESPACE_S
queueListModel::queueListModel(QObject *parent)
    : QAbstractListModel(parent) {
}

int queueListModel::rowCount(const QModelIndex &parent) const {
  return boost::numeric_cast<int>(p_updataQueue.size());
}

QVariant queueListModel::data(const QModelIndex &index, int role) const {
  auto var = QVariant();
  //检查有效性
  if (!index.isValid())
    return var;
  if (index.row() >= p_updataQueue.size())
    return var;

  switch (role) {
    case Qt::DisplayRole:
      break;

    default:
      break;
  }
  return var;
}

QVariant queueListModel::headerData(int section,
                                    Qt::Orientation orientation,
                                    int role) const {
  return QVariant();
}

Qt::ItemFlags queueListModel::flags(const QModelIndex &index) const {
  return 0;
}

bool queueListModel::setData(const QModelIndex &index, const QVariant &value, int role) {
  return false;
}

bool queueListModel::insertRows(int position, int rows, const QModelIndex &index) {
  return false;
}

bool queueListModel::removeRows(int position, int rows, const QModelIndex &index) {
  return false;
}

DOODLE_NAMESPACE_E
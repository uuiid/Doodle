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
}

QVariant queueListModel::headerData(int section,
                                    Qt::Orientation orientation,
                                    int role) const {
}

Qt::ItemFlags queueListModel::flags(const QModelIndex &index) const {
}

bool queueListModel::setData(const QModelIndex &index, const QVariant &value, int role) {
}

bool queueListModel::insertRows(int position, int rows, const QModelIndex &index) {
}

bool queueListModel::removeRows(int position, int rows, const QModelIndex &index) {
}

DOODLE_NAMESPACE_E
#include <lib/ui/model/MotionModel.h>


#include <boost/numeric/conversion/cast.hpp>
#include <QtGui/qcolor.h>
#include <QtGui/qicon.h>

namespace doodle::motion::ui {
MotionModel::MotionModel(QObject *parent)
    : QAbstractListModel(parent),
      p_lists() {
}

int MotionModel::rowCount(const QModelIndex &parent) const {
  return boost::numeric_cast<int>(p_lists.size());
}

QVariant MotionModel::data(const QModelIndex &index, int role) const {
  auto var = QVariant{};
  if (!index.isValid()) return var;
  if (index.row() >= p_lists.size()) return var;

  auto k_data = p_lists.at(index.row());
  switch (role) {
    case Qt::DisplayRole: {
      var = QString::fromStdString(k_data->Title());
      break;
    }
    case Qt::DecorationRole: {
      if (k_data->hasIconFile()) {
        auto k_icon = QIcon{QString::fromStdString(k_data->IconFile().generic_u8string())};
        var         = k_icon.pixmap(QSize{128, 128});
      } else
        var = QColor{Qt::darkRed};
      break;
    }
    case Qt::UserRole: {
      var = QVariant::fromValue(k_data.get());
    }
    default:
      break;
  }
  return var;
}

Qt::ItemFlags MotionModel::flags(const QModelIndex &index) const {
  return Qt::ItemIsEditable | Qt::ItemIsEnabled |
         QAbstractListModel::flags(index);
}

bool MotionModel::setData(const QModelIndex &index, const QVariant &value, int role) {
  if (!index.isValid()) return false;
  if (index.row() >= p_lists.size()) return false;
  if (!value.isValid()) return false;

  auto k_data = p_lists.at(index.row());

  if (value.canConvert<QString>()) {
    if (value.toString().isEmpty()) return false;
    k_data->setTitle(value.toString().toStdString());
  } else if (value.canConvert<QStringList>()) {
    if (value.toStringList().isEmpty()) return false;
    k_data->setInfo(value.toStringList().first().toStdString());
  }
  return true;
}

bool MotionModel::insertRows(int position, int rows, const QModelIndex &index) {
  beginInsertRows(index, position, position + rows - 1);
  for (auto i = 0; i < rows; ++i) {
    auto k_data = std::make_shared<kernel::MotionFile>();
    this->p_lists.insert(this->p_lists.begin() + position + i, k_data);
  }
  endInsertRows();
  return true;
}

bool MotionModel::removeRows(int position, int rows, const QModelIndex &index) {
  if ((position + rows) >= this->p_lists.size()) return false;

  beginRemoveRows(index, position, position + rows - 1);
  for (auto i = 0; i < rows; ++i) {
    auto k_data = this->p_lists.at(position + i);
    auto k_it   = std::find(this->p_lists.begin(), this->p_lists.end(), k_data);
    this->p_lists.erase(k_it);
  }
  endRemoveRows();
  return true;
}

bool MotionModel::insertData(int position, const kernel::MotionFilePtr &data) {
  beginInsertRows(QModelIndex(), position, position + 1);
  this->p_lists.insert(this->p_lists.begin() + position, data);
  endInsertRows();
  return true;
}

void MotionModel::setLists(const std::vector<kernel::MotionFilePtr> &lists) {
  beginResetModel();
  p_lists = lists;
  endResetModel();
}

}  // namespace doodle::motion::ui
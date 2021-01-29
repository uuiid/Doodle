//
// Created by teXiao on 2020/10/14.
//

#include "shotTableModel.h"
#include <loggerlib/Logger.h>
#include < core_Cpp.h>

#include <memory>
#include <boost/numeric/conversion/cast.hpp>

DOODLE_NAMESPACE_S
shotTableModel::shotTableModel(QObject *parent)
    : QAbstractTableModel(parent),
      p_shot_info_ptr_list_(),
      FBRex(std::make_unique<boost::regex>(R"(mp4|avi)")),
      mayaRex(std::make_unique<boost::regex>(R"(ma|ab)")),
      show_mayaex(std::make_unique<boost::regex>(R"(Anm|Animation|export)")),
      show_FBRex(std::make_unique<boost::regex>(R"(FB_|flipbook)")) {
  // shotFileSqlInfo::insertChanged.connect(5, [this]() { this->init(); });
  // shotFileSqlInfo::updateChanged.connect(5, [this]() { this->reInit(); });
}

int shotTableModel::rowCount(const QModelIndex &parent) const {
  return boost::numeric_cast<int>(p_shot_info_ptr_list_.size());
}

int shotTableModel::columnCount(const QModelIndex &parent) const { return 5; }

QVariant shotTableModel::data(const QModelIndex &index, int role) const {
  auto var = QVariant();
  if (!index.isValid()) return var;
  if (index.row() >= p_shot_info_ptr_list_.size()) return var;

  auto shot = p_shot_info_ptr_list_[index.row()];
  if (!shot) {
    return var;
  }
  switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
      if (shot->isInsert()) {
        switch (index.column()) {
          case 0:
            var = QString("v%1").arg(shot->getVersionP(), 4, 10,
                                     QLatin1Char('0'));
            break;
          case 1: {
            auto info = shot->getInfoP();
            if (!info.empty())
              var = DOTOS(info.back());
            break;
          }
          case 2:
            var = shot->getUserQ();
            break;
          case 3:
            var = shot->getSuffixesQ();
            break;
          case 4:
            if (shot->getShotType()) {
              var = shot->getShotType()->getTypeQ();
            }
            break;
          default:
            var = QVariant();
            break;
        }
      } else {
        switch (index.column()) {
          case 1:
            var = "正在上传";
            break;
          case 2:
            var = shot->getUserQ();
            break;
          default:
            var = QVariant();
            break;
        }
      }
      break;
    case Qt::DecorationRole:
      if (shot->isInsert()) {
        if (boost::regex_match(shot->getSuffixes(), *FBRex)) {
          var = QIcon(":/resource/mayaIcon.png");
        } else if (boost::regex_match(shot->getSuffixes(), *mayaRex)) {
          var = QColor("lightcyan");
        }
      }
      break;
    case Qt::ToolTipRole:
      if (shot->isInsert()) {
        if (index.column() == 1) {
          QString tooltip{};
          for (auto &&tex : shot->getInfoP()) {
            tooltip.push_front(DOTOS(tex));
            tooltip.push_front("\n=====================\n");
          }
          var = tooltip;
        }
      }
      break;
    case Qt::BackgroundColorRole:
      if (shot->isInsert()) {
        if (!shot->exist(false))
          var = QColor("darkred");
      }
      break;
    case Qt::UserRole:
      var = QVariant::fromValue(shot.get());
    default:
      break;
  }
  return var;
}

QVariant shotTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
  QString str;
  if (orientation == Qt::Horizontal) {
    switch (section) {
      case 0:
        str = tr("版本");
        break;
      case 1:
        str = tr("信息");
        break;
      case 2:
        str = tr("制作人");
        break;
      case 3:
        str = tr("后缀");
        break;
      case 4:
        str = QString("id");
        break;
      default:
        str = "";
        break;
    }
  } else
    str = QString(section);
  return str;
}

bool shotTableModel::setData(const QModelIndex &index, const QVariant &value, int role) {
  if (!index.isValid()) return false;
  if (index.row() >= p_shot_info_ptr_list_.size()) return false;

  if (index.column() == 1 && role == Qt::EditRole) {
    if (!value.toString().isEmpty() &&
        value.toString().toStdString() !=
            p_shot_info_ptr_list_[index.row()]->getInfoP().back()) {
      DOODLE_LOG_INFO(p_shot_info_ptr_list_[index.row()]->getInfoP().back().c_str());
      p_shot_info_ptr_list_[index.row()]->setInfoP(value.toString().toStdString());
      p_shot_info_ptr_list_[index.row()]->updateSQL();
      dataChanged(index, index);
      return true;
    }
    return false;
  } else if (role == Qt::UserRole) {
    // if (!value.canConvert<shotInfoPtr>()) return false;
    // p_shot_info_ptr_list_[index.row()] = value.value<shotInfoPtr>();
    dataChanged(index, index);
    return true;
  } else {
    return false;
  }
}

Qt::ItemFlags shotTableModel::flags(const QModelIndex &index) const {
  if (index.column() == 1)
    return Qt::ItemIsEditable | Qt::ItemIsEnabled |
           QAbstractTableModel::flags(index);
  else
    return QAbstractTableModel::flags(index);
}

bool shotTableModel::insertRows(int position, int rows,
                                const QModelIndex &parent) {
  beginInsertRows(QModelIndex(), position, position + rows - 1);
  for (int row = 0; row < rows; ++row) {
    p_shot_info_ptr_list_.insert(p_shot_info_ptr_list_.begin() + position,
                                 std::make_shared<shotFileSqlInfo>());
    p_shot_info_ptr_list_[position]->setShot(coreDataManager::get().getShotPtr());
  }
  endInsertRows();
  return true;
}

bool shotTableModel::removeRows(int position, int rows, const QModelIndex &parent) {
  beginRemoveRows(QModelIndex(), position, position + rows - 1);
  for (int row = 0; row < rows; ++row) {
    auto it = p_shot_info_ptr_list_[position];
    if (it)
      it->deleteSQL();
    coreDataManager::get().setShotInfoPtr(nullptr);
    p_shot_info_ptr_list_.erase(p_shot_info_ptr_list_.begin() + position);
  }
  endRemoveRows();
  return true;
}

void shotTableModel::clear() {
  if (p_shot_info_ptr_list_.empty()) return;
  beginResetModel();
  p_shot_info_ptr_list_.clear();
  endResetModel();
}
void shotTableModel::setList(const shotInfoPtrList &list) {
  clear();
  if (list.empty()) {
    return;
  }
  beginInsertRows(QModelIndex(), 0, boost::numeric_cast<int>(list.size()) - 1);
  p_shot_info_ptr_list_ = list;
  endInsertRows();
}

void shotTableModel::doodle_dataChande(const shotInfoPtr &item) {
  auto it = std::find_if(
      p_shot_info_ptr_list_.begin(), p_shot_info_ptr_list_.end(),
      [=](shotInfoPtr &i) -> bool {
        return i == item;
      });
  if (it != p_shot_info_ptr_list_.end()) {
    auto k_index = std::distance(p_shot_info_ptr_list_.begin(), it);

    dataChanged(index(k_index, 0), index(k_index, 4));
  }
}

void shotTableModel::doodle_dataInsert(const shotInfoPtr &item) {
  auto k_size = boost::numeric_cast<int>(p_shot_info_ptr_list_.size());

  if (item->getShot() == coreDataManager::get().getShotPtr()) {
    beginInsertRows(QModelIndex(), k_size, k_size);
    p_shot_info_ptr_list_.push_back(item);
    endInsertRows();
  }
}

DOODLE_NAMESPACE_E

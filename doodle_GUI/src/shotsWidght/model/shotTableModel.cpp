//
// Created by teXiao on 2020/10/14.
//

#include "shotTableModel.h"
#include "Logger.h"
#include <core_doQt.h>

#include <memory>
#include <boost/numeric/conversion/cast.hpp>

DOODLE_NAMESPACE_S
shotTableModel::shotTableModel(QObject *parent)
    : QAbstractTableModel(parent),
      p_shot_info_ptr_list_(),
      p_tmp_shot_info_ptr_list_(),
      FBRex(std::make_unique<boost::regex>(R"(mp4|avi)")),
      mayaRex(std::make_unique<boost::regex>(R"(ma|ab)")),
      show_mayaex(std::make_unique<boost::regex>(R"(Anm|Animation|export)")),
      show_FBRex(std::make_unique<boost::regex>(R"(FB_|flipbook)")) {
  doCore::shotFileSqlInfo::insertChanged.connect([this]() { this->init(); });
  doCore::shotFileSqlInfo::updateChanged.connect([this]() { this->init(); });
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
          case 1:
            var = DOTOS(shot->getInfoP().back());
            break;
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
            if (tex == shot->getInfoP().back()) continue;
            tooltip.append(DOTOS(tex));
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
      var = QVariant::fromValue(shot);
    default:
      break;
  }
  return var;
}
QVariant shotTableModel::headerData(int section, Qt::Orientation orientation,
                                    int role) const {
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

bool shotTableModel::setData(const QModelIndex &index, const QVariant &value,
                             int role) {
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
    if (!value.canConvert<doCore::shotInfoPtr>()) return false;
    p_shot_info_ptr_list_[index.row()] = value.value<doCore::shotInfoPtr>();
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
  //  auto version = p_shot_info_ptr_list_[0]->getVersionP();
  beginInsertRows(QModelIndex(), position, position + rows - 1);
  for (int row = 0; row < rows; ++row) {
    p_shot_info_ptr_list_.insert(p_shot_info_ptr_list_.begin() + position,
                                 std::make_shared<doCore::shotFileSqlInfo>());
    p_shot_info_ptr_list_[position]->setShot(doCore::coreDataManager::get().getShotPtr());
  }
  endInsertRows();
  return true;
}
void shotTableModel::init() {
  clear();
  auto shot = doCore::coreDataManager::get().getShotPtr();
  if (shot) {
    p_tmp_shot_info_ptr_list_ = doCore::shotFileSqlInfo::getAll(shot);
  } else {
    auto eps                  = doCore::coreDataManager::get().getEpisodesPtr();
    p_tmp_shot_info_ptr_list_ = doCore::shotFileSqlInfo::getAll(eps);
  }

  eachOne();
}

void shotTableModel::reInit() {
  auto shot = doCore::coreDataManager::get().getShotPtr();
  for (auto &&x : doCore::shotFileSqlInfo::Instances()) {
    if (x->getShot() == shot) {
      p_tmp_shot_info_ptr_list_.push_back(x->shared_from_this());
    }
  }
  eachOne();
}
void shotTableModel::clear() {
  if (p_shot_info_ptr_list_.empty()) return;
  beginResetModel();
  p_shot_info_ptr_list_.clear();
  p_tmp_shot_info_ptr_list_.clear();
  endResetModel();
}
void shotTableModel::filter(bool useFilter) {
  clear();
  if (useFilter) {
    doCore::shotInfoPtrList list;
    const auto shotTy = doCore::coreDataManager::get().getShotTypePtr();
    const auto shotCl = doCore::coreDataManager::get().getShotClassPtr();
    for (const auto &info_l : p_tmp_shot_info_ptr_list_) {
      if (shotCl && shotTy) {
        if ((shotCl == info_l->getShotclass()) &&
            shotTy == info_l->getShotType())
          list.push_back(info_l);
      } else if (shotCl) {
        if (shotCl == info_l->getShotclass()) list.push_back(info_l);
      } else if (shotTy) {
        if (shotTy == info_l->getShotType()) list.push_back(info_l);
      }
      //      if (
      //          (!shotCl || (shotCl == info_l->getShotclass()))
      //              && (!shotTy || (shotTy == info_l->getShotType()))
      //          ) {
      //        list.push_back(info_l);
      //      }
    }
    setList(list);
  } else {
    eachOne();
  }
}
void shotTableModel::eachOne() {
  std::vector<std::string> show{"Animation", "FB_VFX", "FB_Light", "flipbook"};
  auto listout = p_tmp_shot_info_ptr_list_;

  auto it = std::remove_if(
      listout.begin(), listout.end(), [=](const doCore::shotInfoPtr &ptr) {
        if (ptr && ptr->getShotType()) {
          return !(
              boost::regex_search(ptr->getShotType()->getType(), *show_FBRex) ||
              boost::regex_search(ptr->getShotType()->getType(), *show_mayaex) ||
              boost::regex_search(ptr->getSuffixes(), *mayaRex));
        } else {
          return false;
        }
      });

  doCore::shotInfoPtrList list;
  list.assign(listout.begin(), it);
  listout.clear();
  doCore::shotTypePtrList typelist;
  for (const auto &i : list) {
    if (std::find(typelist.begin(), typelist.end(), i->getShotType()) == typelist.end()) {
      listout.push_back(i);
      typelist.push_back(i->getShotType());
    }
  }

  //  for (const auto &info_l : ) {
  //    auto item = std::find(show.begin(), show.end(),
  //    info_l->getShotType()->getType()); if (item != show.end()) {
  //      list.push_back(info_l);
  //      show.erase(item);
  //    }
  //  }
  setList(listout);
}
void shotTableModel::setList(const doCore::shotInfoPtrList &list) {
  clear();
  if (list.empty()) {
    return;
  }
  beginInsertRows(QModelIndex(), 0, boost::numeric_cast<int>(list.size()) - 1);
  p_shot_info_ptr_list_ = list;
  endInsertRows();
}
void shotTableModel::showAll() {
  setList(p_tmp_shot_info_ptr_list_);
}

DOODLE_NAMESPACE_E

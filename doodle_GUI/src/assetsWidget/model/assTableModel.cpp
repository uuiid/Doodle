//
// Created by teXiao on 2020/10/15.
//

#include "assTableModel.h"

#include < core_Cpp.h>

#include <boost/numeric/conversion/cast.hpp>
#include <memory>

#include "Logger.h"

DOODLE_NAMESPACE_S
assTableModel::assTableModel(QObject *parent)
    : QAbstractTableModel(parent),
      p_ass_info_ptr_list_(),
      mayaRex(std::make_shared<boost::regex>(R"(scenes)")),
      ue4Rex(std::make_shared<boost::regex>(R"(_UE4)")),
      rigRex(std::make_shared<boost::regex>(R"(rig)")) {
  assFileSqlInfo::insertChanged.connect(5, [this](const assInfoPtr &t1) {
    this->doodle_dataInsert(t1);
  });
  assFileSqlInfo::updateChanged.connect(5, [this](const assInfoPtr &t1) {
    this->doodle_dataChande(t1);
  });
}

int assTableModel::rowCount(const QModelIndex &parent) const {
  return boost::numeric_cast<int>(p_ass_info_ptr_list_.size());
}

int assTableModel::columnCount(const QModelIndex &parent) const { return 5; }

QVariant assTableModel::data(const QModelIndex &index, int role) const {
  auto var = QVariant();
  if (!index.isValid()) return var;
  if (index.row() >= p_ass_info_ptr_list_.size()) return var;

  auto ass = p_ass_info_ptr_list_[index.row()];
  switch (role) {
    case Qt::DisplayRole:
      if (ass->isInsert()) {
        switch (index.column()) {
          case 0:
            var =
                QString("v%1").arg(ass->getVersionP(), 4, 10, QLatin1Char('0'));
            break;
          case 1: {
            auto info = ass->getInfoP();
            if (!info.empty())
              var = DOTOS(info.back());
            break;
          }
          case 2:
            var = DOTOS(ass->getUser());
            break;
          case 3:
            var = DOTOS(ass->getSuffixes());
            break;
          case 4:
            if (ass->getAssType()) {
              var = ass->getAssType()->getTypeQ();
            }
            break;
          default:
            var = "";
            break;
        }
      } else {
        switch (index.column()) {
          case 1:
            var = "正在上传";
            break;
          case 2:
            var = DOTOS(ass->getUser());
            break;
          case 3:
            var = "正在上传";
            break;
          default:
            var = "";
            break;
        }
      }
      break;
    case Qt::EditRole:
      if (ass->isInsert()) {
        switch (index.column()) {
          case 0:
            var = ass->getVersionP();
            break;
          case 1:
            var = DOTOS(ass->getInfoP().back());
            break;
          case 2:
            var = DOTOS(ass->getUser());
            break;
          case 3:
            var = DOTOS(ass->getSuffixes());
            break;
          case 4:
            var = ass->getAssType()->getTypeQ();
            break;
          default:
            var = "";
            break;
        }
      }
      break;
    case Qt::DecorationRole:
      if (ass->isInsert()) {
        if (ass->getAssType()) {
          if (boost::regex_match(ass->getAssType()->getTypeS(), *(mayaRex)) &&
              index.column() == 3) {
            var = QIcon(":/resource/mayaIcon.png");
          } else if (boost::regex_match(ass->getAssType()->getTypeS(),
                                        *(ue4Rex)) &&
                     index.column() == 3) {
            var = QIcon(":/resource/ue4Icon.png");
          } else if (boost::regex_match(ass->getAssType()->getTypeS(),
                                        *(rigRex)) &&
                     index.column() == 3) {
            var = QColor("lightblue");
          } else {
            var = QVariant();
          }
        }
      }
      break;
    case Qt::UserRole:
      var = QVariant::fromValue(ass.get());
      break;
    case Qt::BackgroundColorRole: {
      if (ass->isInsert()) {
        if (!ass->exist(false)) var = QColor("darkred");
      }
    } break;
    case Qt::ToolTipRole:
      if (ass->isInsert()) {
        if (index.column() == 1) {
          QString tooltip{};
          for (auto &&tex : ass->getInfoP()) {
            tooltip.push_front(DOTOS(tex));
            tooltip.push_front("\n=====================\n");
          }
          var = tooltip;
        }
      }
      break;
    default:
      var = QVariant();
      break;
  }
  return var;
}

QVariant assTableModel::headerData(int section, Qt::Orientation orientation,
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

bool assTableModel::setData(const QModelIndex &index, const QVariant &value,
                            int role) {
  if (!index.isValid()) return false;
  if (index.row() >= p_ass_info_ptr_list_.size()) return false;

  switch (role) {
    case Qt::EditRole:
      if (index.column() == 1) {
        if (!value.toString().isEmpty() &&
            value.toString().toStdString() !=
                p_ass_info_ptr_list_[index.row()]->getInfoP().back()) {
          DOODLE_LOG_INFO(
              p_ass_info_ptr_list_[index.row()]->getInfoP().back().c_str());
          p_ass_info_ptr_list_[index.row()]->setInfoP(
              value.toString().toStdString());
          p_ass_info_ptr_list_[index.row()]->updateSQL();
          dataChanged(index, index);
          return true;
        } else {
          break;
        }
      } else {
        break;
      }
    default:
      break;
  }
  return false;
}

Qt::ItemFlags assTableModel::flags(const QModelIndex &index) const {
  if (index.column() == 1)
    return Qt::ItemIsEditable | Qt::ItemIsEnabled |
           QAbstractTableModel::flags(index);
  else
    return QAbstractTableModel::flags(index);
}
bool assTableModel::insertRows(int position, int rows,
                               const QModelIndex &parent) {
  beginInsertRows(QModelIndex(), position, position + rows - 1);
  //  beginInsertColumns(QModelIndex(), 0, 4);
  for (int row = 0; row < rows; ++row) {
    p_ass_info_ptr_list_.insert(p_ass_info_ptr_list_.begin() + position,
                                std::make_shared<assFileSqlInfo>());
    p_ass_info_ptr_list_[position]->setAssClass(coreDataManager::get().getAssClassPtr());
  }
  //  endInsertColumns();
  endInsertRows();
  return true;
}
bool assTableModel::removeRows(int position, int rows,
                               const QModelIndex &parent) {
  beginRemoveRows(QModelIndex(), position, position + rows - 1);
  for (int row = 0; row < rows; ++row) {
    auto ass = p_ass_info_ptr_list_[position];
    if (ass) ass->deleteSQL();
    p_ass_info_ptr_list_.erase(p_ass_info_ptr_list_.begin() + position);
  }
  endRemoveRows();
  return true;
}

void assTableModel::clear() {
  if (p_ass_info_ptr_list_.empty()) return;
  beginResetModel();
  p_ass_info_ptr_list_.clear();
  endResetModel();
}
void assTableModel::setList(assInfoPtrList &list) {
  clear();
  if (list.empty()) {
    return;
  }
  beginInsertRows(QModelIndex(), 0, boost::numeric_cast<int>(list.size()) - 1);
  p_ass_info_ptr_list_ = list;
  endInsertRows();
}

void assTableModel::doodle_dataChande(const assInfoPtr &i) {
  auto it =
      std::find_if(p_ass_info_ptr_list_.begin(), p_ass_info_ptr_list_.end(),
                   [=](assInfoPtr &d) -> bool {
                     return d == i;
                   });

  if (it != p_ass_info_ptr_list_.end()) {
    auto k_index = std::distance(p_ass_info_ptr_list_.begin(), it);

    dataChanged(index(k_index, 0), index(k_index, 4));
  }
}

void assTableModel::doodle_dataInsert(const assInfoPtr &i) {
  auto k_size = boost::numeric_cast<int>(p_ass_info_ptr_list_.size());
  //如果是当前资产的更改就插入
  if (i->getAssClass() == coreDataManager::get().getAssClassPtr()) {
    beginInsertRows(QModelIndex(), k_size, k_size);
    p_ass_info_ptr_list_.push_back(i);
    endInsertRows();
  }
}

DOODLE_NAMESPACE_E

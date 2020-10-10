/*
 * @Author: your name
 * @Date: 2020-10-10 10:26:01
 * @LastEditTime: 2020-10-10 15:58:55
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit  
 * @FilePath: \Doodle\doodle_GUI\src\fileClassShotWidget.cpp
 */
#include "fileClassShotWidget.h"

#include "src/fileclass.h"
#include "Logger.h"

DOODLE_NAMESPACE_S
fileClassShotModel::fileClassShotModel(QObject *parent)
    : QAbstractListModel(parent),
      list_fileClass(),
      p_shot(nullptr) {
}

int fileClassShotModel::rowCount(const QModelIndex &parent) const {
  return list_fileClass.size();
}

QVariant fileClassShotModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid())
    return QVariant();
  if (index.row() >= list_fileClass.size())
    return QVariant();
  if (role == Qt::DisplayRole || role == Qt::EditRole) {
    return list_fileClass[index.row()]->getFileclass_str();
  } else {
    return QVariant();
  }
}

doCore::fileClassPtr fileClassShotModel::dataRow(const QModelIndex &index) const {
  if (!index.isValid())
    return nullptr;
  if (!index.row() >= list_fileClass.size())
    return nullptr;
  return list_fileClass[index.row()];
}

QVariant fileClassShotModel::headerData(int section, Qt::Orientation orientation, int role) const {
  if (role != Qt::DisplayRole)
    return QVariant();

  if (orientation == Qt::Horizontal)
    return QStringLiteral("Column %1").arg(section);
  else
    return QStringLiteral("Row %1").arg(section);
}

Qt::ItemFlags fileClassShotModel::flags(const QModelIndex &index) const {
  if (!index.isValid())
    return Qt::ItemIsEnabled;

  if (list_fileClass[index.row()]->isInsert())
    return QAbstractListModel::flags(index);
  else
    return Qt::ItemIsEnabled | Qt::ItemIsEnabled | QAbstractListModel::flags(index);
}

bool fileClassShotModel::setData(const QModelIndex &index, const QVariant &value, int role) {
  if (index.isValid() && role == Qt::EditRole) {
    //确认没有重复的fileclass
    bool isHas = false;
    for (auto &&i : list_fileClass) {
      if (value.toString() == i->getFileclass_str() || i->isInsert()) {
        isHas = true;
        break;
      }
    }

    if (isHas)
      return false;
    else {
      DOODLE_LOG_CRIT << "注意将fileclass提交到数据库";
      list_fileClass[index.row()]->setFileclass(value.toString());
      list_fileClass[index.row()]->setShot(p_shot);
      list_fileClass[index.row()]->insert();

      emit dataChanged(index, index, {role});
      return true;
    }
  }
  return false;
}

bool fileClassShotModel::insertRows(int position, int rows, const QModelIndex &index) {
  beginInsertRows(index, position, position + rows - 1);

  for (int row = 0; row < rows; ++row) {
    DOODLE_LOG_INFO << "插入新的fileclass镜头";
    list_fileClass.insert(position, doCore::fileClassPtr(new doCore::fileClass));
  }
  endInsertRows();
  return true;
}

bool fileClassShotModel::removeRows(int position, int rows, const QModelIndex &index) {
  beginRemoveRows(QModelIndex(), position, position + rows - 1);

  for (int row = 0; row < rows; ++row) {
    DOODLE_LOG_INFO << "去除队列中的fileclass镜头";
    list_fileClass.remove(position);
  }

  endRemoveRows();
  return true;
}

void fileClassShotModel::init(const doCore::shotPtr &shot) {
  doCore::fileClassPtrList fileClassPtrList = doCore::fileClass::getAll(shot);
  if (!list_fileClass.isEmpty()) {
    //开始删除模型中的fileclass
    beginRemoveRows(QModelIndex(), 0, list_fileClass.size() - 1);
    list_fileClass.clear();
    endRemoveRows();

  }
  beginInsertRows(QModelIndex(), 0, fileClassPtrList.size());
  list_fileClass = fileClassPtrList;
  endInsertRows();

}

/* --------------------------------- 自定义小部件 --------------------------------- */
fileClassShotWidget::fileClassShotWidget(QWidget *parent)
    : QListView(parent),
      p_model(nullptr),
      p_fileClass_menu(nullptr),
      p_shot(nullptr) {
  p_model = new fileClassShotModel(this);

  setModel(p_model);

  setStatusTip(tr("使用右键直接添加部门类型"));

  connect(this, &fileClassShotWidget::clicked,
          this, &fileClassShotWidget::_doodle_fileclass_emit);
}

fileClassShotWidget::~fileClassShotWidget() {
}

void fileClassShotWidget::init(const doCore::shotPtr &shot) {
  p_shot = shot;
  p_model->init(shot);
}

void fileClassShotWidget::fileClassShotEmitted(const doCore::fileClassPtr &fc_) {

}

void fileClassShotWidget::insertFileClass() {

}

void fileClassShotWidget::_doodle_fileclass_emit(const QModelIndex &index) {

}

void fileClassShotWidget::contextMenuEvent(QContextMenuEvent *event) {
  QAbstractScrollArea::contextMenuEvent(event);
}

DOODLE_NAMESPACE_E

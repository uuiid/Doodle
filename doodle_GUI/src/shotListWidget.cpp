#include "shotListWidget.h"

#include "src/shot.h"

#include "Logger.h"

#include <QSpinBox>
#include <QHBoxLayout>
#include <QComboBox>
#include <QMessageBox>
#include <QMenu>
#include <QAction>

#include <QContextMenuEvent>

DOODLE_NAMESPACE_S

shotListModel::shotListModel(QObject *parent)
    : QAbstractListModel(parent),
      shotlist(),
      p_episodes(nullptr) {
}

shotListModel::~shotListModel() {
}

void shotListModel::init(const doCore::episodesPtr &episodes_) {
  doCore::shotPtrList tmp_shot_list = doCore::shot::getAll(episodes_);
  if (!shotlist.isEmpty()) {
    beginRemoveRows(QModelIndex(), 0, shotlist.size() - 1);
    shotlist.clear();
    endRemoveRows();

    beginInsertRows(QModelIndex(), 0, tmp_shot_list.size() - 1);
    shotlist = tmp_shot_list;
    endInsertRows();
  } else {
    beginInsertRows(QModelIndex(), 0, tmp_shot_list.size());
    shotlist = tmp_shot_list;
    endInsertRows();
  }
  p_episodes = episodes_;
}

int shotListModel::rowCount(const QModelIndex &parent) const {
  return shotlist.size();
}

QVariant shotListModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid())
    return QVariant();

  if (index.row() >= shotlist.size())
    return QVariant();

  if (role == Qt::DisplayRole || role == Qt::EditRole) {
    return shotlist[index.row()]->getShotAndAb_str();
  } else {
    return QVariant();
  }
}

doCore::shotPtr shotListModel::dataRaw(const QModelIndex &index) const {
  if (!index.isValid())
    return nullptr;

  return shotlist[index.row()];
}

QVariant shotListModel::headerData(int section,
                                   Qt::Orientation orientation,
                                   int role) const {
  if (role != Qt::DisplayRole)
    return QVariant();

  if (orientation == Qt::Horizontal)
    return QStringLiteral("Column %1").arg(section);
  else
    return QStringLiteral("Row %1").arg(section);
}

Qt::ItemFlags shotListModel::flags(const QModelIndex &index) const {
  if (!index.isValid())
    return Qt::ItemIsEnabled;

  if (shotlist[index.row()]->isInsert())
    return QAbstractListModel::flags(index);
  else
    return Qt::ItemIsEditable | Qt::ItemIsEnabled | QAbstractListModel::flags(index);
}

bool shotListModel::setData(const QModelIndex &index, const QVariant &value, int role) {
  QMap infoMap = value.toMap();
  if (index.isValid() && role == Qt::EditRole) {
    //确认镜头不重复和没有提交
    //这个函数不设置AB镜
    bool isHasShot = false;
    for (auto &&x : shotlist) {
      if (infoMap["shot"].toInt() == x->getShot() || x->isInsert()) {
        isHasShot = true;
      }
    }

    if (isHasShot)
      return false;
    else {
      shotlist[index.row()]->setShot(infoMap["shot"].toInt(), infoMap["shotAb"].toString());
      shotlist[index.row()]->setEpisodes(p_episodes);
      shotlist[index.row()]->insert();
      emit dataChanged(index, index, {role});
      return true;
    }
  }
  return false;
}

bool shotListModel::insertRows(int position, int rows, const QModelIndex &index) {
  beginInsertRows(index, position, position + rows - 1);

  for (int row = 0; row < rows; ++row) {
    shotlist.insert(position, doCore::shotPtr(new doCore::shot));
  }
  endInsertRows();
  return true;
}

bool shotListModel::removeRows(int position, int rows, const QModelIndex &index) {
  beginRemoveRows(index, position, position + rows - 1);
  for (int row = 0; row < rows; ++row) {
    shotlist.remove(position);
  }
  endRemoveRows();
  return true;
}

/* -------------------------------- 自定义小部件 -------------------------------- */

shotEditWidget::shotEditWidget(QWidget *parent) : QWidget(parent),
                                                  p_b_hboxLayout(nullptr),
                                                  p_spin(nullptr),
                                                  p_combox(nullptr) {

  //基本布局
  p_b_hboxLayout = new QHBoxLayout(this);
  p_b_hboxLayout->setSpacing(0);

  p_spin = new QSpinBox(this);
  p_combox = new QComboBox(this);

  p_b_hboxLayout->addWidget(p_spin);
  p_b_hboxLayout->addWidget(p_combox);

  this->setLayout(p_b_hboxLayout);
  p_b_hboxLayout->setContentsMargins(0, 0, 0, 0);
  p_b_hboxLayout->setSpacing(0);

  p_combox->addItem(QString(""));
  for (auto &&i : doCore::shot::e_shotAB_list) {
    p_combox->addItem(QString::fromStdString(i));
  }
}

shotEditWidget::~shotEditWidget() {
}

QMap<QString, QVariant> shotEditWidget::value() {
  QMap<QString, QVariant> map;

  map["shot"] = p_spin->value();
  map["shotAb"] = p_combox->currentText();
  return map;
}

void shotEditWidget::setValue(const QMap<QString, QVariant> &value) {
  p_spin->setValue(value["shot"].toInt());
  p_combox->setCurrentText(value["shotAb"].toString());
}

/* ---------------------------------- 自定义委托 --------------------------------- */

shotIntEnumDelegate::shotIntEnumDelegate(QObject *parent) : QStyledItemDelegate(parent) {
}

shotIntEnumDelegate::~shotIntEnumDelegate() {
}

QWidget *shotIntEnumDelegate::createEditor(QWidget *parent,
                                           const QStyleOptionViewItem &option,
                                           const QModelIndex &index) const {
  auto *shotedit = new shotEditWidget(parent);

  return shotedit;
}

void shotIntEnumDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const {
  auto *shotedit = static_cast<shotEditWidget *>(editor);
  QMap<QString, QVariant> map;
  map["shot"] = 0;
  map["shotAb"] = "";
  shotedit->setValue(map);
}

void shotIntEnumDelegate::setModelData(QWidget *editor,
                                       QAbstractItemModel *model,
                                       const QModelIndex &index) const {
  auto *shotedit = static_cast<shotEditWidget *>(editor);

  QMap<QString, QVariant> data = shotedit->value();
  QMessageBox::StandardButton box = QMessageBox::information(static_cast<QWidget *>(this->parent()),
                                                             tr("警告:"),
                                                             tr("将镜头 sc%1%2 提交到服务器").arg(data["shot"].toInt(),
                                                                                         4,
                                                                                         10,
                                                                                         QLatin1Char('0')).arg(data["shotAb"].toString()),
                                                             QMessageBox::Yes | QMessageBox::Cancel);
  if (box == QMessageBox::Yes)
    model->setData(index, data, Qt::EditRole);
  else
    model->removeRow(index.row(), index);
}

void shotIntEnumDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                                               const QModelIndex &index) const {
  editor->setGeometry(option.rect);
}

/* ------------------------------- 自定义shot小部件 ------------------------------- */
shotLsitWidget::shotLsitWidget(QWidget *parent)
    : QListView(parent),
      p_model(nullptr),
      p_delegate(nullptr),
      p_shot_menu(nullptr),
      p_episodes(nullptr) {
  p_model = new shotListModel(this);
  p_delegate = new shotIntEnumDelegate(this);

  setModel(p_model);
  setItemDelegate(p_delegate);

  setStatusTip(tr("镜头栏 右键添加镜头"));

  connect(this, &shotLsitWidget::clicked,
          this, &shotLsitWidget::_doodle_shot_emit);
}

shotLsitWidget::~shotLsitWidget() {};

void shotLsitWidget::init(const doCore::episodesPtr &episodes_) {
  p_episodes = episodes_;
  p_model->init(episodes_);
}

void shotLsitWidget::insertShot() {
  int raw = selectionModel()->currentIndex().row() + 1;
  p_model->insertRow(raw, QModelIndex());

  setCurrentIndex(p_model->index(raw));
  edit(p_model->index(raw));
}

void shotLsitWidget::_doodle_shot_emit(const QModelIndex &index) {
  emit shotEmit(p_model->dataRaw(index));
}

void shotLsitWidget::contextMenuEvent(QContextMenuEvent *event) {
  p_shot_menu = new QMenu(this);
  if (p_episodes) {
    auto *action = new QAction(this);

    connect(action, &QAction::triggered,
            this, &shotLsitWidget::insertShot);
    action->setText(tr("添加镜头"));
    action->setStatusTip(tr("添加镜头"));
    p_shot_menu->addAction(action);
  }
  p_shot_menu->move(event->globalPos());
  p_shot_menu->show();
}


DOODLE_NAMESPACE_E
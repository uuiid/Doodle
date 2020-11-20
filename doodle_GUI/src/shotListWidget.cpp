#include <core_doQt.h>

#include "Logger.h"
#include "shotListModel.h"

#include "shotListWidget.h"

#include <QSpinBox>
#include <QHBoxLayout>
#include <QComboBox>
#include <QMessageBox>
#include <QMenu>
#include <QContextMenuEvent>
#include <QtWidgets/QApplication>
DOODLE_NAMESPACE_S

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

shotEditWidget::~shotEditWidget() = default;

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

//void shotEditWidget::mousePressEvent(QMouseEvent *event) {
//  auto ptr = qApp->widgetAt(event->globalPos());
//  if (ptr != p_spin || ptr != p_combox) {
//    editingFinished();
//  }
//  QWidget::mousePressEvent(event);
//}

/* ---------------------------------- 自定义委托 --------------------------------- */

shotIntEnumDelegate::shotIntEnumDelegate(QObject *parent) : QStyledItemDelegate(parent) {
}

shotIntEnumDelegate::~shotIntEnumDelegate() = default;

QWidget *shotIntEnumDelegate::createEditor(QWidget *parent,
                                           const QStyleOptionViewItem &option,
                                           const QModelIndex &index) const {
  auto *shotedit = new shotEditWidget(parent);

  return shotedit;
}

void shotIntEnumDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const {
  auto *shotedit = static_cast<shotEditWidget *>(editor);
  auto map = index.data(Qt::EditRole).toMap();
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
  if (box == QMessageBox::Yes) {
    if (!model->setData(index, data, Qt::EditRole)) {
      model->removeRow(index.row(), QModelIndex());
    }
  }
  else
    model->removeRow(index.row(), QModelIndex());
  
}

void shotIntEnumDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                                               const QModelIndex &index) const {
  editor->setGeometry(option.rect);
}

/* ------------------------------- 自定义shot小部件 ------------------------------- */
shotListWidget::shotListWidget(QWidget *parent)
    : QListView(parent),
      p_model_(nullptr),
      p_delegate(nullptr),
      p_shot_menu(nullptr) {
  p_delegate = new shotIntEnumDelegate(this);

  setItemDelegate(p_delegate);

  setStatusTip(tr("镜头栏 右键添加镜头"));

  connect(this, &shotListWidget::clicked,
          this, &shotListWidget::_doodle_shot_emit);
}

shotListWidget::~shotListWidget() = default;
void shotListWidget::insertShot() {
  int raw = selectionModel()->currentIndex().row() + 1;
  p_model_->insertRow(raw, QModelIndex());

  setCurrentIndex(p_model_->index(raw));
  edit(p_model_->index(raw));
}

void shotListWidget::_doodle_shot_emit(const QModelIndex &index) {
  doCore::coreDataManager::get().setShotPtr(
      index.data(Qt::UserRole).value<doCore::shotPtr>()
      );
  emit initEmit();
}

void shotListWidget::contextMenuEvent(QContextMenuEvent *event) {
  p_shot_menu = new QMenu(this);
  if (doCore::coreDataManager::get().getEpisodesPtr()) {
    auto *action = new QAction(this);

    connect(action, &QAction::triggered,
            this, &shotListWidget::insertShot);
    action->setText(tr("添加镜头"));
    action->setStatusTip(tr("添加镜头"));
    p_shot_menu->addAction(action);
  }
  p_shot_menu->move(event->globalPos());
  p_shot_menu->show();
}
void shotListWidget::setModel(QAbstractItemModel *model) {
  auto p_model = dynamic_cast<shotListModel *>(model);
  if (p_model)
    p_model_ = p_model;
  QAbstractItemView::setModel(model);
}

DOODLE_NAMESPACE_E
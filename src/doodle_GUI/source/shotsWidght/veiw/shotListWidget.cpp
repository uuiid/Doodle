#include "shotListWidget.h"

#include < core_Cpp.h>

#include <loggerlib/Logger.h>
#include <doodle_GUI/source/shotsWidght/model/shotListModel.h>

#include <QtCore/QString>
#include <QSpinBox>
#include <QHBoxLayout>
#include <QComboBox>
#include <QMessageBox>
#include <QMenu>
#include <QContextMenuEvent>
#include <QtWidgets/QApplication>
#include <QtWidgets/QInputDialog>

#include <boost/numeric/conversion/cast.hpp>
DOODLE_NAMESPACE_S

/* -------------------------------- 自定义小部件
 * -------------------------------- */

shotEditWidget::shotEditWidget(QWidget *parent)
    : QWidget(parent),
      p_b_hboxLayout(nullptr),
      p_spin(nullptr),
      p_combox(nullptr) {
  //基本布局
  p_b_hboxLayout = new QHBoxLayout(this);
  p_b_hboxLayout->setSpacing(0);

  p_spin = new QSpinBox(this);
  p_spin->setRange(1, 999);
  p_combox = new QComboBox(this);

  p_b_hboxLayout->addWidget(p_spin);
  p_b_hboxLayout->addWidget(p_combox);

  this->setLayout(p_b_hboxLayout);
  p_b_hboxLayout->setContentsMargins(0, 0, 0, 0);
  p_b_hboxLayout->setSpacing(0);

  p_combox->addItem(QString(""));
  for (auto &&i : shot::e_shotAB_list) {
    p_combox->addItem(QString::fromStdString(i));
  }
}

shotEditWidget::~shotEditWidget() = default;

QMap<QString, QVariant> shotEditWidget::value() {
  QMap<QString, QVariant> map;

  map["shot"]   = p_spin->value();
  map["shotAb"] = p_combox->currentText();
  return map;
}

void shotEditWidget::setValue(const QMap<QString, QVariant> &value) {
  p_spin->setValue(value["shot"].toInt());
  p_combox->setCurrentText(value["shotAb"].toString());
}

// void shotEditWidget::mousePressEvent(QMouseEvent *event) {
//  auto ptr = qApp->widgetAt(event->globalPos());
//  if (ptr != p_spin || ptr != p_combox) {
//    editingFinished();
//  }
//  QWidget::mousePressEvent(event);
//}

/* ---------------------------------- 自定义委托
 * --------------------------------- */

shotIntEnumDelegate::shotIntEnumDelegate(QObject *parent)
    : QStyledItemDelegate(parent) {}

shotIntEnumDelegate::~shotIntEnumDelegate() = default;

QWidget *shotIntEnumDelegate::createEditor(QWidget *parent,
                                           const QStyleOptionViewItem &option,
                                           const QModelIndex &index) const {
  auto *shotedit = new shotEditWidget(parent);

  return shotedit;
}

void shotIntEnumDelegate::setEditorData(QWidget *editor,
                                        const QModelIndex &index) const {
  auto *shotedit = static_cast<shotEditWidget *>(editor);
  auto map       = index.data(Qt::EditRole).toMap();
  shotedit->setValue(map);
}

void shotIntEnumDelegate::setModelData(QWidget *editor,
                                       QAbstractItemModel *model,
                                       const QModelIndex &index) const {
  auto *shotedit               = static_cast<shotEditWidget *>(editor);
  auto map                     = index.data(Qt::EditRole).toMap();
  QMap<QString, QVariant> data = shotedit->value();
  if (map == data) {
    model->setData(index, data, Qt::EditRole);
    return;
  }

  QMessageBox::StandardButton box = QMessageBox::information(
      static_cast<QWidget *>(this->parent()), tr("警告:"),
      tr("将镜头 sc%1%2 提交到服务器")
          .arg(data["shot"].toInt(), 4, 10, QLatin1Char('0'))
          .arg(data["shotAb"].toString()),
      QMessageBox::Yes | QMessageBox::Cancel);

  if (box == QMessageBox::Yes) {
    if (!model->setData(index, data, Qt::EditRole)) {
      model->removeRow(index.row(), QModelIndex());
    }
  } else
    model->removeRow(index.row(), QModelIndex());
}

void shotIntEnumDelegate::updateEditorGeometry(
    QWidget *editor, const QStyleOptionViewItem &option,
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

  setEditTriggers(QAbstractItemView::NoEditTriggers);
  setItemDelegate(p_delegate);

  setStatusTip(tr("镜头栏 右键添加镜头"));

  connect(this, &shotListWidget::clicked, this,
          &shotListWidget::_doodle_clicked_emit);
}

shotListWidget::~shotListWidget() = default;
void shotListWidget::insertShot() {
  int raw = selectionModel()->currentIndex().row() + 1;
  p_model_->insertRow(raw, QModelIndex());

  setCurrentIndex(p_model_->index(raw));
  edit(p_model_->index(raw));
}

void shotListWidget::insertShotBatch() {
  auto k_start = QInputDialog::getInt(this, tr("开始镜头"), tr("开始镜头号"), 1, 1, 999);
  auto k_end   = QInputDialog::getInt(this, tr("结束镜头"), tr("结束镜头号"), 0, 1, 999);
  auto k_point = selectionModel()->currentIndex().row();
  if (k_start <= k_end) {
    p_model_->insertRows(k_point, k_end - k_start + 1, QModelIndex());
    for (size_t i = 0; i <= k_end - k_start; ++i) {
      auto index = p_model_->index(boost::numeric_cast<int>(k_point + i));
      p_model_->setData(index,
                        QMap<QString, QVariant>{{"shot", i + k_start}, {"shotAb", ""}},
                        Qt::EditRole);
    }
  }
}

void shotListWidget::deleteShot() {
  if (selectionModel()->hasSelection()) {
    if (shotFileSqlInfo::Instances().empty()) {
      p_model_->removeRow(selectionModel()->currentIndex().row());
    } else {
      DOODLE_LOG_INFO(" 这个条目内还有内容,  无法删除: ");
      QMessageBox::warning(this, tr("注意: "),
                           tr("这个条目内还有内容,  无法删除"));
    }
  }
}

void shotListWidget::_doodle_clicked_emit(const QModelIndex &index) {
  auto info = index.data(Qt::UserRole).value<shot *>();
  if (info)
    chickItem(info->shared_from_this());
}

void shotListWidget::contextMenuEvent(QContextMenuEvent *event) {
  p_shot_menu = new QMenu(this);
  if (coreDataManager::get().getEpisodesPtr()) {
    auto action = new QAction(this);

    connect(action, &QAction::triggered, this, &shotListWidget::insertShot);
    action->setText(tr("添加镜头"));
    action->setStatusTip(tr("添加镜头"));
    p_shot_menu->addAction(action);

    //批量添加镜头
    auto k_add_shot_batch = new QAction();
    connect(k_add_shot_batch, &QAction::triggered,
            this, &shotListWidget::insertShotBatch);
    k_add_shot_batch->setText(tr("批量添加镜头"));
    p_shot_menu->addAction(k_add_shot_batch);

    if (selectionModel()->hasSelection()) {
      //添加镜头同步
      auto shot_ = selectionModel()
                       ->currentIndex()
                       .data(Qt::UserRole)
                       .value<shot *>();
      if (shot_) {
        auto synShot = p_shot_menu->addAction("同步镜头");
        connect(synShot, &QAction::triggered, this, &shotListWidget::synShot);
      }
      p_shot_menu->addSeparator();
      //添加镜头删除和修改
      auto k_shot_delete = p_shot_menu->addAction(tr("删除镜头"));
      connect(k_shot_delete, &QAction::triggered,
              this, &shotListWidget::deleteShot);
      auto k_shot_modify = p_shot_menu->addAction(tr("修改镜头"));
      connect(k_shot_modify, &QAction::triggered, [=]() {
        edit(selectionModel()->currentIndex());
      });
    }
  }
  p_shot_menu->move(event->globalPos());
  p_shot_menu->show();
}
void shotListWidget::setModel(QAbstractItemModel *model) {
  auto p_model = dynamic_cast<shotListModel *>(model);
  if (p_model) p_model_ = p_model;
  QAbstractItemView::setModel(model);
}

void shotListWidget::synShot() {
  auto shot_ = selectionModel()
                   ->currentIndex()
                   .data(Qt::UserRole)
                   .value<shot *>();
  auto eps_ptr = coreDataManager::get().getEpisodesPtr();
  coreSet::getSet().setSyneps(eps_ptr->getEpisdes());
  ueSynArchive().syn(eps_ptr, shot_->shared_from_this());
}
DOODLE_NAMESPACE_E
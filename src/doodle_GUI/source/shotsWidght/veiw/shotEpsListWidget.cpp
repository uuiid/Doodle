#include "shotEpsListWidget.h"

#include <loggerlib/Logger.h>

#include <doodle_GUI/source/shotsWidght/model/shotEpsListModel.h>
#include <core_Cpp.h>

#include <QSpinBox>
#include <QMenu>
#include <QContextMenuEvent>
#include <QMessageBox>

#include <iostream>

DOODLE_NAMESPACE_S

//------------------------------------------------------------------------------------
//                                         集数编辑类
//------------------------------------------------------------------------------------

episodesintDelegate::episodesintDelegate(QObject *parent)
    : QStyledItemDelegate(parent) {
}

episodesintDelegate::~episodesintDelegate() = default;

QWidget *episodesintDelegate::createEditor(QWidget *parent,
                                           const QStyleOptionViewItem &option,
                                           const QModelIndex &index) const {
  QSpinBox *editor = new QSpinBox(parent);
  editor->setFrame(false);
  editor->setMinimum(1);
  editor->setMaximum(9999);
  return editor;
}

void episodesintDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const {
  auto int_value = index.data(Qt::EditRole).toInt();

  auto *spinbox = static_cast<QSpinBox *>(editor);
  spinbox->setValue(int_value);
}

void episodesintDelegate::setModelData(QWidget *editor,
                                       QAbstractItemModel *model,
                                       const QModelIndex &index) const {
  auto *spinbox = static_cast<QSpinBox *>(editor);
  spinbox->interpretText();
  int value = spinbox->value();

  if (index.data(Qt::EditRole).toInt() == value) {
    model->setData(index, value, Qt::EditRole);
    return;
  }

  QMessageBox::StandardButton box = QMessageBox::information(static_cast<QWidget *>(this->parent()),
                                                             tr("警告:"), tr("将第 %1 集提交到服务器").arg(value),
                                                             QMessageBox::Yes | QMessageBox::Cancel);
  if (box == QMessageBox::Yes) {
    if (!model->setData(index, value, Qt::EditRole)) {
      model->removeRow(index.row(), QModelIndex());
    }
  } else
    model->removeRow(index.row(), QModelIndex());
}

void episodesintDelegate::updateEditorGeometry(QWidget *editor,
                                               const QStyleOptionViewItem &option,
                                               const QModelIndex &index) const {
  editor->setGeometry(option.rect);
}

/* ---------------------------------- 集数小部件 --------------------------------- */

shotEpsListWidget::shotEpsListWidget(QWidget *parent)
    : QListView(parent),
      p_episodesListDelegate(nullptr),
      p_eps_Menu(nullptr) {
  p_episodesListDelegate = new episodesintDelegate(this);

  setEditTriggers(QAbstractItemView::EditKeyPressed);
  setItemDelegate(p_episodesListDelegate);

  setStatusTip(tr("集数栏 注意不要添加错误的集数"));

  connect(this, &shotEpsListWidget::clicked,
          this, &shotEpsListWidget::_doodle_clicked_emit);
}

shotEpsListWidget::~shotEpsListWidget() = default;

void shotEpsListWidget::insertEpisodes() {
  int raw = selectionModel()->currentIndex().row();
  model()->insertRow(raw, QModelIndex());

  //设置当前行的选择
  setCurrentIndex(p_episodesListModel->index(raw));
  edit(p_episodesListModel->index(raw));
}

void shotEpsListWidget::contextMenuEvent(QContextMenuEvent *event) {
  if (p_eps_Menu) {
    p_eps_Menu->clear();
  } else {
    p_eps_Menu = new QMenu(this);
  }

  auto add_eps = new QAction();
  connect(add_eps, &QAction::triggered,
          this, &shotEpsListWidget::insertEpisodes);
  add_eps->setText(tr("添加集数"));
  add_eps->setStatusTip(tr("添加集数"));
  p_eps_Menu->addAction(add_eps);

  if (selectionModel()->hasSelection()) {
    auto eps_ptr = selectionModel()->currentIndex().data(Qt::UserRole).value<episodes *>();
    if (!eps_ptr) return;

    auto createMove = new QAction();
    createMove->setText(tr("制作整集拍屏"));
    connect(createMove, &QAction::triggered,
            this, [this, eps_ptr]() {
              chickItem(eps_ptr->shared_from_this());
              this->creatEpsMov();
            });
    p_eps_Menu->addAction(createMove);

    auto syneps = new QAction();
    syneps->setText(tr("同步集数"));
    connect(syneps, &QAction::triggered,
            this, [eps_ptr]() {
              std::make_shared<ueSynArchive>()->syn(eps_ptr->shared_from_this(), nullptr);
            });
    // syneps->setToolTip();
    p_eps_Menu->addAction(syneps);

    p_eps_Menu->addSection(tr("注意"));

    auto modify_eps = new QAction();
    connect(modify_eps, &QAction::triggered,
            [=]() {
              //设置当前行的选择
              edit(selectionModel()->currentIndex());
            });
    modify_eps->setText(tr("修改集数"));
    p_eps_Menu->addAction(modify_eps);

    auto k_deleteEpsiodes = new QAction();
    connect(k_deleteEpsiodes, &QAction::triggered, [=]() { this->deleteEpsiodes(); });
    k_deleteEpsiodes->setText(tr("删除集数"));
    p_eps_Menu->addAction(k_deleteEpsiodes);
  }
  p_eps_Menu->move(event->globalPos());
  p_eps_Menu->show();
}

void shotEpsListWidget::_doodle_clicked_emit(const QModelIndex &index) {
  auto info = index.data(Qt::UserRole).value<episodes *>();
  if (info)
    chickItem(info->shared_from_this());
}
void shotEpsListWidget::setModel(QAbstractItemModel *model) {
  auto p_model = dynamic_cast<shotEpsListModel *>(model);
  if (p_model) p_episodesListModel = p_model;
  QAbstractItemView::setModel(model);
}
void shotEpsListWidget::creatEpsMov() {
  auto p_instance = shot::Instances();
  if (!p_instance.empty()) {
    auto shotInfo = std::make_shared<shotFileSqlInfo>();

    const auto &kEps = selectionModel()->currentIndex().data(Qt::UserRole).value<episodes *>();
    if (kEps) {
      shotInfo->setEpisdes(kEps->shared_from_this());

      auto move = std::make_unique<movieEpsArchive>(shotInfo);
      move->update();
    }
  } else {
    QMessageBox::warning(this, tr("注意: "),
                         tr("这个集数上没有上传拍屏"));
    auto data = selectionModel()->currentIndex().data(Qt::UserRole);
    auto eps  = data.value<episodes *>();
    DOODLE_LOG_INFO(" 这个集数上没有上传拍屏: " << eps->getEpisdes_str());
  }
}

void shotEpsListWidget::deleteEpsiodes() {
  if (selectionModel()->hasSelection())
    if (shot::Instances().empty() && shotFileSqlInfo::Instances().empty()) {
      p_episodesListModel->removeRow(selectionModel()->currentIndex().row());
    } else {
      auto data = selectionModel()->currentIndex().data(Qt::UserRole);
      auto eps  = data.value<episodes *>();
      DOODLE_LOG_INFO(" 这个条目内还有内容,  无法删除: " << eps->getEpisdes_str());
      QMessageBox::warning(this, tr("注意: "),
                           tr("这个条目内还有内容,  无法删除"));
    }
}
DOODLE_NAMESPACE_E
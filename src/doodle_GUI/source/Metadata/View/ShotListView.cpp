// Fill out your copyright notice in the Description page of Project Settings.
#include <doodle_GUI/source/Metadata/View/ShotListView.h>
#include <doodle_GUI/source/Metadata/Model/ShotListmodel.h>

#include <corelib/core_Cpp.h>

#include <magic_enum.hpp>

#include <QtWidgets/QMenu>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QMessageBox>
#include <QtGui/QContextMenuEvent>

namespace doodle {

ShotListView::ShotListView(QWidget* parent)
    : QListView(parent),
      p_shot_model() {
}

void ShotListView::contextMenuEvent(QContextMenuEvent* event) {
  auto menu = new QMenu(this);

  if (!p_shot_model) {
    p_shot_model = dynamic_cast<ShotListModel*>(model());
    if (!p_shot_model)
      return;
  }

  auto k_add_shot = menu->addAction("添加镜头");
  connect(k_add_shot, &QAction::triggered, this, &ShotListView::addShot);
  if (selectionModel()->hasSelection()) {
    auto k_add_shot_ab = menu->addAction("添加ab镜");
    connect(k_add_shot_ab, &QAction::triggered, this, &ShotListView::addShotAb);

    auto k_delete_shot = menu->addAction("删除镜头");
    connect(k_delete_shot, &QAction::triggered, this, &ShotListView::removeShot);
  }
  menu->move(event->globalPos());
  menu->show();
}

void ShotListView::addShot() {
  auto k_shot_int = QInputDialog::getInt(this, tr("镜头号"), tr("镜头号"), 0, 0, 9999);
  auto k_shot     = std::make_shared<Shot>(k_shot_int, std::string{}, p_shot_model->Episodes_());
  auto k_position = int{0};
  if (selectionModel()->hasSelection()) {
    k_position = selectionModel()->currentIndex().row() + 1;
  }

  p_shot_model->addShot_(k_position, k_shot);
}

void ShotListView::addShotAb() {
  QStringList k_str_list{};
  for (auto k_shot_enum : magic_enum::enum_names<Shot::ShotAbEnum>())
    k_str_list.append(QString::fromStdString(std::string{k_shot_enum}));

  if (selectionModel()->hasSelection()) {
    auto k_shot_ab = QInputDialog::getItem(this, tr("ab镜"), tr("ab镜: "), k_str_list);
    auto k_data    = selectionModel()->currentIndex().data(Qt::UserRole).value<Shot*>();
    if (!k_data) {
      QMessageBox::warning(this, QString::fromUtf8("注意: "), tr("无法获取数据"));
      return;
    }
    auto k_position = selectionModel()->currentIndex().row() + 1;

    auto k_shot = std::make_shared<Shot>(k_data->Shot_(),
                                         k_shot_ab.toStdString(),
                                         p_shot_model->Episodes_());
    p_shot_model->addShot_(k_position, k_shot);
  }
}

void ShotListView::removeShot() {
  if (selectionModel()->hasSelection()) {
    auto k_position = selectionModel()->currentIndex().row();
    p_shot_model->removeShot(k_position);
  }
}

//这个是模态对话框
ShotListDialog::ShotListDialog(QWidget* parent)
    : QDialog(parent),
      p_episodes(),
      p_shots(),
      p_shot_model() {
  auto layout = new QGridLayout(this);
  auto k_eps  = new QSpinBox();
  k_eps->setValue(1);
  p_shot_model     = new ShotListModel(this);
  auto k_shot_vidw = new ShotListView();
  k_shot_vidw->setModel(p_shot_model);

  layout->addWidget(k_eps, 0, 0, 1, 1);
  layout->addWidget(k_shot_vidw, 1, 0, 1, 1);

  p_episodes = std::make_shared<Episodes>(1);

  auto& set      = coreSet::getSet();
  const auto end = set.gettUe4Setting().ShotEnd();
  for (auto i = set.gettUe4Setting().ShotStart(); i < end; ++i)
    p_shots.emplace_back(std::make_shared<Shot>(i, std::string{}, p_episodes));

  p_shot_model->setList(p_shots, p_episodes);

  connect(k_eps, qOverload<int>(&QSpinBox::valueChanged), this,
          [this](int value) { this->p_episodes->setEpisodes_(value); });

  setLayout(layout);
  setWindowTitle(tr("创建镜头"));
}

std::tuple<EpisodesPtr, std::vector<ShotPtr>> ShotListDialog::getShotList(QWidget* parent) {
  ShotListDialog k_dialog(parent);
  k_dialog.exec();
  return {k_dialog.p_shot_model->Episodes_(), k_dialog.p_shot_model->Shots_()};
}

}  // namespace doodle
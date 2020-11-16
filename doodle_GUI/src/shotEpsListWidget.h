/*
 * @Author: your name
 * @Date: 2020-09-28 14:46:53
 * @LastEditTime: 2020-10-09 15:00:48
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_GUI\src\shotEpsListWidget.h
 */
#pragma once

#include "doodle_global.h"
#include "core_global.h"


#include <QAbstractListModel>
#include <QStyledItemDelegate>
#include <QListView>

DOODLE_NAMESPACE_S

/**
 * @description: 自定义集数委托
 */
class episodesintDelegate : public QStyledItemDelegate {
 Q_OBJECT

 private:
  /* data */
 public:
  explicit episodesintDelegate(QObject *parent = nullptr);
  ~episodesintDelegate() override;
  //创建一个提供编辑的小部件
  QWidget *createEditor(QWidget *parent,
                        const QStyleOptionViewItem &option,
                        const QModelIndex &index) const override;

  void setEditorData(QWidget *editor, const QModelIndex &index) const override;
  void setModelData(QWidget *editor,
                    QAbstractItemModel *model,
                    const QModelIndex &index) const override;
  void updateEditorGeometry(QWidget *editor,
                            const QStyleOptionViewItem &option,
                            const QModelIndex &index) const override;
};

class shotEpsListWidget : public QListView {
 Q_OBJECT
 public:
  explicit shotEpsListWidget(QWidget *parent = nullptr);
  ~shotEpsListWidget() override;

  void setModel(QAbstractItemModel *model) override;
 public slots:
  void init();
 signals:
  void initEmit();

 private:
  shotEpsListModel *p_episodesListModel;
  episodesintDelegate *p_episodesListDelegate;

  QMenu *p_eps_Menu;
 private slots:
  void insertEpisodes();
  void _doodle_episodes_emit(const QModelIndex &index);

 protected:
  void contextMenuEvent(QContextMenuEvent *event) override;
};

DOODLE_NAMESPACE_E
//
// Created by teXiao on 2020/10/16.
//

#pragma once

#include <doodle_global.h>
#include <core_global.h>

#include <QStyledItemDelegate>
#include <QListView>

#include <boost/signals2.hpp>
DOODLE_NAMESPACE_S

class fileTypeAssDelegate : public QStyledItemDelegate {
  Q_OBJECT
 public:
  explicit fileTypeAssDelegate(QObject *parent = nullptr);

  QWidget *createEditor(QWidget *parent,
                        const QStyleOptionViewItem &option,
                        const QModelIndex &index) const override;

  void setEditorData(QWidget *editor, const QModelIndex &index) const override;
  void setModelData(QWidget *editor, QAbstractItemModel *model,
                    const QModelIndex &index) const override;
  void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                            const QModelIndex &index) const override;
};

class assTypeWidget : public QListView {
  Q_OBJECT
 public:
  explicit assTypeWidget(QWidget *parent = nullptr);

  void setModel(QAbstractItemModel *model) override;

  boost::signals2::signal<void(const assTypePtr &, const filterState &)> chickItem;

 private:
  //m模型指针
  assTypeModel *p_model_;
  //菜单
  QMenu *p_menu_;

 private Q_SLOTS:
  void inserttype();
  void _doodle_type_emit(const QModelIndex &index);

 protected:
  void mousePressEvent(QMouseEvent *event) override;
  //void contextMenuEvent(QContextMenuEvent *event) override;
};

DOODLE_NAMESPACE_E

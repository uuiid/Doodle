/*
 * @Author: your name
 * @Date: 2020-09-30 14:05:57
 * @LastEditTime: 2020-10-10 14:31:42
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_GUI\src\shotListWidget.h
 */
#pragma once

#include "doodle_global.h"

#include "core_global.h"

#include <QStyledItemDelegate>
#include <QListView>

DOODLE_NAMESPACE_S


/**
 * @description: 自定义小部件, 用来修改shot使用
 */
class shotEditWidget : public QWidget {
 Q_OBJECT

 public:
  explicit shotEditWidget(QWidget *parent = nullptr);
  ~shotEditWidget() override;

  QMap<QString, QVariant> value();
  void setValue(const QMap<QString, QVariant> &value);
 signals:
  void editingFinished();
 private:
  QSpinBox *p_spin;
  QComboBox *p_combox;

  QHBoxLayout *p_b_hboxLayout;

 //protected:
 // void mousePressEvent(QMouseEvent *event) override;
};

/**
 * @description: 自定义委托类型
 */
class shotIntEnumDelegate : public QStyledItemDelegate {
 Q_OBJECT

 public:
  explicit shotIntEnumDelegate(QObject *parent = nullptr);
  ~shotIntEnumDelegate() override;

  QWidget *createEditor(QWidget *parent,
                        const QStyleOptionViewItem &option,
                        const QModelIndex &index) const override;

  void setEditorData(QWidget *editor, const QModelIndex &index) const override;
  void setModelData(QWidget *editor, QAbstractItemModel *model,
                    const QModelIndex &index) const override;
  void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                            const QModelIndex &index) const override;
};

/* ------------------------------- 自定义shot小部件 ------------------------------- */

class shotListWidget : public QListView {
 Q_OBJECT

 public:
  explicit shotListWidget(QWidget *parent = nullptr);
  ~shotListWidget() override;

  void setModel(QAbstractItemModel *model) override;
 signals:
  void initEmit();

 private:
  //私有变量
  //模型
  shotListModel *p_model_;
  //自定义委托
  shotIntEnumDelegate *p_delegate;
  //上下文菜单
  QMenu *p_shot_menu;

 private slots:
  //添加镜头号
  void insertShot();
  //私有的镜头点击发射事件
  void _doodle_shot_emit(const QModelIndex &index);

 protected:
  void contextMenuEvent(QContextMenuEvent *event) override;
};
DOODLE_NAMESPACE_E
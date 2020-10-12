//
// Created by teXiao on 2020/10/12.
//

#pragma once

#include "doodle_global.h"

#include "core_global.h"

#include <QAbstractListModel>
#include <QStyledItemDelegate>
#include <QListView>

DOODLE_NAMESPACE_S
//-----------------------------------自定义模型---------------------------------------------//
class fileTypeShotModel : public QAbstractListModel {
 Q_OBJECT
 public:
  explicit fileTypeShotModel(QObject *parent = nullptr);
  ~fileTypeShotModel() override;
  //返回总行数
  [[nodiscard]] int rowCount(const QModelIndex &parent) const override;

  [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
  [[nodiscard]] doCore::fileTypePtr daraRow(const QModelIndex &index) const;

  //返回标头
  [[nodiscard]] QVariant headerData(int section,
                                    Qt::Orientation Orientation,
                                    int role) const override;
  [[nodiscard]] Qt::ItemFlags flags(const QModelIndex &index) const override;
  //设置数据
  bool setData(const QModelIndex &index, const QVariant &value, int role) override;

  //插入数据
  bool insertRows(int position, int rows, const QModelIndex &index) override;
  //删除数据
  bool removeRows(int position, int rows, const QModelIndex &index) override;

 public slots:
  void init(const doCore::fileClassPtr &file_class_ptr);
  void clear();
 private:
  doCore::fileTypePtrList p_type_ptr_list_;
  doCore::fileClassPtr p_class_ptr_;
};

//-----------------------------------自定义委托---------------------------------------------//
class fileTypeShotDelegate : public QStyledItemDelegate {
 Q_OBJECT

 public:
  explicit fileTypeShotDelegate(QObject *parent = nullptr);
  ~fileTypeShotDelegate() override;

  QWidget *createEditor(QWidget *parent,
                        const QStyleOptionViewItem &option_view_item,
                        const QModelIndex &index) const override;
  void setEditorData(QWidget *editor,
                     const QModelIndex &index) const override;
  void setModelData(QWidget *editor, QAbstractItemModel *model,
                    const QModelIndex &index) const override;
  void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                            const QModelIndex &index) const override;
};

//-----------------------------------自定义小部件---------------------------------------------//
class fileTypeShotWidget : public QListView {
 Q_OBJECT
 public:
  explicit fileTypeShotWidget(QWidget *parent = nullptr);
  ~fileTypeShotWidget() override;

 public slots:
  void init(const doCore::fileClassPtr &file_class_ptr);
  void clear();
 signals:
  void typeEmit(const doCore::fileTypePtr &file_type_ptr);

 private:
  //模型
  fileTypeShotModel *p_file_type_shot_model_;
  //委托
  fileTypeShotDelegate *p_file_type_shot_delegate_;
  //上下文菜单
  QMenu *p_menu_;
  //上一级发射出来的指针
  doCore::fileClassPtr p_file_class_ptr_;
 private slots:
  //添加filetype
  void insertFileType();
  //发射fileType
  void _doodle_type_emit(const QModelIndex &index);
 protected:
  void contextMenuEvent(QContextMenuEvent *event) override;
};

DOODLE_NAMESPACE_E


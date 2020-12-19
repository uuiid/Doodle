/*
 * @Author: your name
 * @Date: 2020-11-23 11:35:18
 * @LastEditTime: 2020-11-30 13:41:13
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_GUI\src\assetsWidget\assWidget.h
 */
#pragma once
#include <doodle_global.h>
class QSortFilterProxyModel;
DOODLE_NAMESPACE_S

class assWidght : public QWidget {
  Q_OBJECT
 public:
  explicit assWidght(QWidget *parent = nullptr);
 public Q_SLOTS:
  void refresh();
 private Q_SLOTS:
  void setFilterRegExp(const QString &filter);

 private:
  //这些都是资产变量
  QGridLayout *p_ass_layout_;
  assDepModel *p_ass_dep_model_;
  assClassModel *p_ass_class_model_;
  assTableModel *p_ass_table_model_;

  assTypeModel *p_ass_type_model_;  //这个作为过滤器
  //代理模型
  QSortFilterProxyModel *p_ass_sortfilter_model_;
  assDepWidget *p_ass_dep_widget_;
  assClassWidget *p_ass_class_widget_;
  assTypeWidget *p_ass_type_widget_;
  assTableWidght *p_ass_info_widght_;
};

DOODLE_NAMESPACE_E
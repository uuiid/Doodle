/*
 * @Author: your name
 * @Date: 2020-11-16 19:06:04
 * @LastEditTime: 2020-12-14 17:34:21
 * @LastEditors: your name
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_GUI\src\shotsWidght\model\shotTableModel.h
 */
//
// Created by teXiao on 2020/10/14.
//
#pragma once

#include "doodle_global.h"

#include "core_global.h"
#include <QAbstractTableModel>
#include <boost/regex.hpp>

DOODLE_NAMESPACE_S
class shotTableModel : public QAbstractTableModel {
  Q_OBJECT
 public:
  explicit shotTableModel(QObject *parent = nullptr);

  [[nodiscard]] int rowCount(const QModelIndex &parent) const override;
  [[nodiscard]] int columnCount(const QModelIndex &parent) const override;
  [[nodiscard]] QVariant data(const QModelIndex &index,
                              int role) const override;
  [[nodiscard]] QVariant headerData(int section,
                                    Qt::Orientation orientation,
                                    int role) const override;
  //设置数据
  bool setData(const QModelIndex &index, const QVariant &value,
               int role) override;
  [[nodiscard]] Qt::ItemFlags flags(const QModelIndex &index) const override;
  //修改数据
  bool insertRows(int position, int rows, const QModelIndex &parent) override;

  void init();
  void reInit();
  void filter(bool useFilter);
  void clear();
  void showAll();

 private:
  void eachOne();
  void setList(const doCore::shotInfoPtrList &list);

 private:
  doCore::shotInfoPtrList p_shot_info_ptr_list_;
  doCore::shotInfoPtrList p_tmp_shot_info_ptr_list_;
  std::unique_ptr<boost::regex> mayaRex;
  std::unique_ptr<boost::regex> FBRex;
  std::unique_ptr<boost::regex> show_FBRex;
  std::unique_ptr<boost::regex> show_mayaex;
};

DOODLE_NAMESPACE_E

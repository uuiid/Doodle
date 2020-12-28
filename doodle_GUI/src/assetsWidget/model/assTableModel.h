/*
 * @Author: your name
 * @Date: 2020-11-16 19:05:27
 * @LastEditTime: 2020-11-28 15:16:32
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_GUI\src\assTableModel.h
 */
//
// Created by teXiao on 2020/10/15.
//
#pragma once

#include "doodle_global.h"
#include "core_global.h"
#include <QAbstractTableModel>
#include <boost/regex.hpp>
DOODLE_NAMESPACE_S

class assTableModel : public QAbstractTableModel {
  Q_OBJECT
 public:
  explicit assTableModel(QObject *parent = nullptr);

  [[nodiscard]] int rowCount(const QModelIndex &parent) const override;
  [[nodiscard]] int columnCount(const QModelIndex &parent) const override;
  [[nodiscard]] QVariant data(const QModelIndex &index,
                              int role) const override;
  [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation,
                                    int role) const override;
  //设置数据
  bool setData(const QModelIndex &index, const QVariant &value,
               int role) override;
  [[nodiscard]] Qt::ItemFlags flags(const QModelIndex &index) const override;
  //修改数据
  bool insertRows(int position, int rows, const QModelIndex &parent) override;
  bool removeRows(int position, int rows, const QModelIndex &parent) override;

  void init();
  void reInit();
  void clear();

 private:
  void setList(assInfoPtrList &list);

 private:
  assInfoPtrList p_ass_info_ptr_list_;
  std::shared_ptr<boost::regex> mayaRex;
  std::shared_ptr<boost::regex> ue4Rex;
  std::shared_ptr<boost::regex> rigRex;
};

DOODLE_NAMESPACE_E

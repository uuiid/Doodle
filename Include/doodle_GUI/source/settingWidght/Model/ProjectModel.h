#pragma once
#include <doodle_GUI/doodle_global.h>
#include <QtCore/QAbstractListModel>

namespace doodle {
class ProjectModel : public QAbstractListModel {
  Q_OBJECT
  std::vector<ProjectPtr> p_data;

 public:
  ProjectModel(QObject *parent = nullptr);

  [[nodiscard]] int rowCount(const QModelIndex &parent) const override;
  [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
  //设置编辑标识
  [[nodiscard]] Qt::ItemFlags flags(const QModelIndex &index) const override;
  //设置数据
  bool setData(const QModelIndex &index, const QVariant &value, int role) override;

  //插入数据
  bool insertRows(int position, int rows, const QModelIndex &index) override;
  //删除数据
  bool removeRows(int position, int rows, const QModelIndex &index) override;

  void init(const decltype(p_data) &data);
};

}  // namespace doodle
#pragma once

#include "doodle_global.h"

#include "core_global.h"

#include <QAbstractListModel>
#include <QStyledItemDelegate>
#include <QListView>

DOODLE_NAMESPACE_S

//这个时镜头的自定义模型
class shotListModel : public QAbstractListModel
{
    Q_OBJECT
private:
    doCore::shotPtrList shotlist;

public:
    shotListModel(QObject *parent = nullptr);
    ~shotListModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    //设置编辑标识
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    //设置数据
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    //插入数据
    bool insertRows(int position, int rows, const QModelIndex &index = QModelIndex()) override;
    //删除数据
    bool removeRows(int position, int rows, const QModelIndex &index = QModelIndex()) override;
public slots:
    void init(doCore::episodesPtr &episodes_);
};

DOODLE_NAMESPACE_E
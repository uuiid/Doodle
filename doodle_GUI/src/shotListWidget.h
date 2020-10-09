/*
 * @Author: your name
 * @Date: 2020-09-30 14:05:57
 * @LastEditTime: 2020-10-09 11:51:42
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_GUI\src\shotListWidget.h
 */
#pragma once

#include "doodle_global.h"

#include "core_global.h"

#include <QAbstractListModel>
#include <QStyledItemDelegate>
#include <QListView>




DOODLE_NAMESPACE_S

/**
 * @description: 这个时镜头的自定义模型
 */
class shotListModel : public QAbstractListModel
{
    Q_OBJECT
private:
    doCore::shotPtrList shotlist;

public:
    shotListModel(QObject *parent = nullptr);
    ~shotListModel();

    /**
     * @description: 
     * @param {type} 
     * @return {type} 
     */
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    //设置编辑标识
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    //设置数据(内部使用 QMap[shot] 和 QMap[shotAb] 获得传入信息)
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    //插入数据
    bool insertRows(int position, int rows, const QModelIndex &index = QModelIndex()) override;
    //删除数据
    bool removeRows(int position, int rows, const QModelIndex &index = QModelIndex()) override;
public slots:
    void init(doCore::episodesPtr &episodes_);
};
/**
 * @description: 自定义小部件, 用来修改shot使用
 */
class shotEditWidget : public QWidget
{
    Q_OBJECT
    
public:
    shotEditWidget(QWidget *parent = nullptr);
    virtual ~shotEditWidget(){};

    QMap<QString, QVariant> value() const;
    void setValue(const QMap<QString, QVariant> &value);

private:
    QMap<QString, QVariant> p_map_value;

    QSpinBox *p_spin;

    QWidget * centralWidget;
    QHBoxLayout * p_b_hboxLayout;
    
};


/**
 * @description: 自定义委托类型
 */
class shotIntEnumDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    shotIntEnumDelegate(const QObject *parent = nullptr);
    virtual ~shotIntEnumDelegate();

    QWidget *createEditor(QWidget *parent,
                          const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;

    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                              const QModelIndex &index) const override;
};
DOODLE_NAMESPACE_E
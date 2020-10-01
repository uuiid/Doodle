/*
 * @Author: your name
 * @Date: 2020-09-28 14:46:53
 * @LastEditTime: 2020-10-01 19:49:23
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_GUI\src\episodesListWidget.h
 */
#pragma once

#include "doodle_global.h"
#include "core_global.h"

#include <QAbstractListModel>
#include <QStyledItemDelegate>
#include <QListView>

DOODLE_NAMESPACE_S
/**
 * @description: 自定义集数模型
 */
class episodesListModel : public QAbstractListModel
{
    Q_OBJECT
private:
    doCore::episodesPtrList eplist;

public:
    episodesListModel(QObject *parent = nullptr);
    ~episodesListModel();

    //返回总行数
    int rowCount(const QModelIndex &parent = QModelIndex()) const override ;
    //返回数据
    QVariant data(const QModelIndex &index,int role) const override;
    doCore::episodesPtr dataRaw(const QModelIndex &index) const;
    //返回标题
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    
    //设置是否编辑标识
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    //设置数据
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    //添加数据
    bool insertRows(int position, int rows, const QModelIndex &index = QModelIndex()) override;
    bool removeRows(int position, int rows, const QModelIndex &index = QModelIndex()) override;
public slots:    
    //刷新函数
    void init();
};

/**
 * @description: 自定义集数委托
 */
class episodesintDelegate : public QStyledItemDelegate
{
    Q_OBJECT

private:
    /* data */
public:
    episodesintDelegate(QObject *parent = nullptr);
    ~episodesintDelegate();
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


class episodesListWidget : public QListView
{
    Q_OBJECT
public:
    episodesListWidget(QWidget *parent = nullptr);
    ~episodesListWidget();
signals:
    void episodesEmit(const doCore::episodesPtr &episodes);

private:
    episodesListModel * p_episodesListModel;
    episodesintDelegate * p_episodesListDelegate;

    QMenu * p_eps_Menu;
private slots:
    void insertEpisodes();
    void _doodle_episodes_emit(const QModelIndex &index);

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;
};

DOODLE_NAMESPACE_E
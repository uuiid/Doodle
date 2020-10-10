/*
 * @Author: your name
 * @Date: 2020-10-10 10:25:56
 * @LastEditTime: 2020-10-10 14:36:12
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_GUI\src\fileClassShotWidget.h
 */
#pragma once

#include "doodle_global.h"
#include "core_global.h"

#include <QListView>
#include <QAbstractListModel>
#include <QStyledItemDelegate>

DOODLE_NAMESPACE_S

/* ---------------------------------- 自定义模型 --------------------------------- */

class fileClassShotModel : public QAbstractListModel
{
    Q_OBJECT
private:
    doCore::fileClassPtrList list_fileClass;
    doCore::shotPtr p_shot;

public:
    explicit fileClassShotModel(QObject *parent = nullptr);
    ~fileClassShotModel() override = default;;

    //返回总行数
    int rowCount(const QModelIndex &parent) const override;
    //返回数据
    QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]] doCore::fileClassPtr dataRow(const QModelIndex &index) const;
    //返回标题
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    //返回设置标识
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    //设置数据
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    //添加和删除数据
    bool insertRows(int position, int rows, const QModelIndex &index) override;
    bool removeRows(int position, int rows, const QModelIndex &index) override;

public slots:
    void init(const doCore::shotPtr &shot);
};

class fileClassShotWidget : public QListView
{
    Q_OBJECT

public:
    explicit fileClassShotWidget(QWidget *parent = nullptr);
    ~fileClassShotWidget() override = default;

    void init(const doCore::shotPtr &shot);
signals:
    void fileClassShotEmitted(const doCore::fileClassPtr &fc_);

public slots:
    //添加fileclass
    void insertFileClass();
    //私有化fileclass发射
    void _doodle_fileclass_emit(const QModelIndex &index);

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    //私有变量
    //模型
    fileClassShotModel *p_model;

    //上下文菜单
    QMenu *p_fileClass_menu;

    //保存上一个小部件发射出来的集数指针
    doCore::shotPtr p_shot;
};

DOODLE_NAMESPACE_E
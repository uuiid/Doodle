#include "shotListWidget.h"

#include "src/shot.h"

#include <QSpinBox>
#include <QHBoxLayout>

DOODLE_NAMESPACE_S

shotListModel::shotListModel(QObject *parent)
    : QAbstractListModel(parent),
      shotlist()
{
}

shotListModel::~shotListModel()
{
}

void shotListModel::init(doCore::episodesPtr &episodes_)
{
    shotlist = doCore::shot::getAll(episodes_);
}

QWidget* shotIntEnumDelegate::createEditor(QWidget *parent,
                          const QStyleOptionViewItem &option,
                          const QModelIndex &index) const 
{
    
}

int shotListModel::rowCount(const QModelIndex &parent) const
{
    return shotlist.size();
}

QVariant shotListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.row() >= shotlist.size())
        return QVariant();

    if (role == Qt::DisplayRole || role == Qt::EditRole)
    {
        return shotlist[index.row()]->getShot_str();
    }
    else
    {
        return QVariant();
    }
}

QVariant shotListModel::headerData(int section,
                                   Qt::Orientation orientation,
                                   int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal)
        return QStringLiteral("Column %1").arg(section);
    else
        return QStringLiteral("Row %1").arg(section);
}

Qt::ItemFlags shotListModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;

    if (shotlist[index.row()]->isInsert())
        return QAbstractListModel::flags(index);
    else
        return Qt::ItemIsEditable | Qt::ItemIsEnabled | QAbstractListModel::flags(index);
}

bool shotListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    QMap infoMap = value.toMap();
    if (index.isValid() && role == Qt::EditRole)
    {
        //确认镜头不重复和没有提交  
        //这个函数不设置AB镜
        bool isHasShot = false;
        for (auto &&x : shotlist)
        {
            if (infoMap["shot"].toInt() == x->getShot() || x->isInsert())
            {
                isHasShot = true;
            }
        }

        if (isHasShot)
            return false;
        else
        {
            shotlist[index.row()]->setShot(infoMap["shot"].toInt(),infoMap["shotAb"].toString());
            shotlist[index.row()]->insert();
            emit dataChanged(index, index, {role});
            return true;
        }
    }
    return false;
}

bool shotListModel::insertRows(int position, int rows, const QModelIndex &index)
{
    beginInsertRows(index, position, position + rows - 1);

    for (int row = 0; row < rows; ++ row)
    {       
        shotlist.insert(position, doCore::shotPtr(new doCore::shot));
    }
    endInsertRows();
    return true;
}

bool shotListModel::removeRows(int position, int rows, const QModelIndex & index)
{
    beginRemoveRows(index,position, position + rows - 1);
    for (int row = 0; row <rows ; ++row)
    { 
        shotlist.remove(position);
    }
    endRemoveRows();
    return true;
}

shotEditWidget::shotEditWidget(QWidget *parent):QWidget(parent)
{
    //中央小部件
    centralWidget = new QWidget(this);
    //基本布局
    p_b_hboxLayout = new QHBoxLayout(centralWidget);
    p_spin = new QSpinBox(this);

}

QMap<QString,QVariant> shotEditWidget::value() const
{
    return p_map_value;
}

void shotEditWidget::setValue(const QMap<QString,QVariant> &value)
{
    if(value.contains("shot") && value.contains("shotAb"))
    {
        p_map_value["shot"] = value["shot"];
        p_map_value["shotAb"] = value["shotAb"];
    }
}
DOODLE_NAMESPACE_E
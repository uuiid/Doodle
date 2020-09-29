#include "episodesListWidget.h"

#include "src/episodes.h"

#include <QSpinBox>

#include <iostream>

DOODLE_NAMESPACE_S

episodesListModel::episodesListModel(QObject *parent) : QAbstractListModel(parent), eplist()
{
    init();
}

episodesListModel::~episodesListModel()
{
}

void episodesListModel::init()
{
    eplist = doCore::episodes::getAll();
}

int episodesListModel::rowCount(const QModelIndex &parent) const
{
    return eplist.size();
}

QVariant episodesListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.row() >= eplist.size())
        return QVariant();

    if (role == Qt::DisplayRole || role == Qt::EditRole)
    {
        return eplist[index.row()]->getEpisdes_str();
    }
    else
    {
        return QVariant();
    }
}

QVariant episodesListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal)
        return QStringLiteral("Column %1").arg(section);
    else
        return QStringLiteral("Row %1").arg(section);
}

Qt::ItemFlags episodesListModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;
    return Qt::ItemIsEditable | Qt::ItemIsEnabled | QAbstractListModel::flags(index);
}

bool episodesListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.isValid() && role == Qt::EditRole)
    {
        bool isHasEps = false;
        for (auto &&x : eplist)
        {
            if (value.toInt() == x->getEpisdes())
                isHasEps = true;
        }
        if (!isHasEps)
            eplist[index.row()]->setEpisdes(value.toInt());
        emit dataChanged(index, index, {role});
        return true;
    }
    return false;
}

bool episodesListModel::insertRows(int position, int rows, const QModelIndex &index)
{
    beginInsertRows(QModelIndex(), position, position + rows - 1);

    eplist.insert(position, doCore::episodesPtr(new doCore::episodes));
    for (int row = 0; row < rows; ++row)
    {
        std::cout << position << " " << row << std::endl;
    }
    endInsertRows();
    return true;
}

//------------------------------------------------------------------------------------
//集数编辑类
//------------------------------------------------------------------------------------

episodesintDelegate::episodesintDelegate(QObject *parent) : QStyledItemDelegate(parent)
{
}

episodesintDelegate::~episodesintDelegate()
{
}

QWidget *episodesintDelegate::createEditor(QWidget *parent,
                                           const QStyleOptionViewItem &option,
                                           const QModelIndex &index) const
{
    QSpinBox *editor = new QSpinBox(parent);
    editor->setFrame(false);
    editor->setMinimum(1);
    editor->setMaximum(9999);
    return editor;
}

void episodesintDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QString name = index.model()->data(index, Qt::EditRole).toString();
    int value = name.right(name.size() - 2).toInt();

    QSpinBox *spinbox = static_cast<QSpinBox *>(editor);
    spinbox->setValue(value);
}

void episodesintDelegate::setModelData(QWidget *editor,
                                       QAbstractItemModel *model,
                                       const QModelIndex &index) const
{
    QSpinBox *spinbox = static_cast<QSpinBox *>(editor);
    spinbox->interpretText();
    int value = spinbox->value();

    model->setData(index, value, Qt::EditRole);
}

void episodesintDelegate::updateEditorGeometry(QWidget *editor,
                                               const QStyleOptionViewItem &option,
                                               const QModelIndex &index) const
{
    editor->setGeometry(option.rect);
}

//----------------------------------------------------------------
//集数小部件
//----------------------------------------------------------------

episodesListWidget::episodesListWidget(QWidget *parent) : 
QListView(parent), 
p_episodesListModel(new episodesListModel),
p_episodesListDelegate(new episodesintDelegate)
{
    p_episodesListModel->setParent(this);
    p_episodesListDelegate->setParent(this);

    setModel(p_episodesListModel);
    setItemDelegate(p_episodesListDelegate);
}

episodesListWidget::~episodesListWidget()
{

}
DOODLE_NAMESPACE_E
#include "episodesListWidget.h"

#include "src/episodes.h"

#include <QSpinBox>
#include <QMenu>
#include <QContextMenuEvent>
#include <QMessageBox>

#include <iostream>

DOODLE_NAMESPACE_S

episodesListModel::episodesListModel(QObject *parent)
    : QAbstractListModel(parent), eplist()
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

doCore::episodesPtr episodesListModel::dataRaw(const QModelIndex &index) const
{
    if (!index.isValid())
        return nullptr;

    if (index.row() >= eplist.size())
        return nullptr;

    return eplist[index.row()];
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

    if (eplist[index.row()]->isInsert())
        return QAbstractListModel::flags(index);
    else
        return Qt::ItemIsEditable | Qt::ItemIsEnabled | QAbstractListModel::flags(index);
}

bool episodesListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.isValid() && role == Qt::EditRole)
    {
        //确认镜头不重复和没有提交
        bool isHasEps = false;
        for (auto &&x : eplist)
        {
            if (value.toInt() == x->getEpisdes() || x->isInsert())
            {
                isHasEps = true;
                break;
            }
        }
        if (isHasEps)
            return false;
        else
        {
            eplist[index.row()]->setEpisdes(value.toInt());
            eplist[index.row()]->insert();
            emit dataChanged(index, index, {role});
            return true;
        }
    }
    return false;
}

bool episodesListModel::insertRows(int position, int rows, const QModelIndex &index)
{
    beginInsertRows(index, position, position + rows - 1);
    for (int row = 0; row < rows; ++row)
    {
        std::cout << position << " " << row << std::endl;
        eplist.insert(position, doCore::episodesPtr(new doCore::episodes));
    }
    endInsertRows();
    return true;
}

bool episodesListModel::removeRows(int position, int rows, const QModelIndex &index)
{
    beginRemoveRows(index, position, position + rows - 1);
    for (int row = 0; row < rows; ++row)
    {
        eplist.remove(position);
    }
    endRemoveRows();
    return true;
}
//------------------------------------------------------------------------------------
//集数编辑类
//------------------------------------------------------------------------------------

episodesintDelegate::episodesintDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
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

    QMessageBox::StandardButton box = QMessageBox::information(static_cast<QWidget *>(this->parent()),
                                                               tr("警告:"), tr("将第 %1 集提交到服务器").arg(value),
                                                               QMessageBox::Yes | QMessageBox::Cancel);
    if (box == QMessageBox::Yes)
        model->setData(index, value, Qt::EditRole);
    else
        model->removeRow(index.row(), index);
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

episodesListWidget::episodesListWidget(QWidget *parent)
    : QListView(parent),
      p_episodesListModel(nullptr),
      p_episodesListDelegate(nullptr),
      p_eps_Menu(nullptr)
{
    p_episodesListModel = new episodesListModel(this);
    p_episodesListDelegate = new episodesintDelegate(this);

    setModel(p_episodesListModel);
    setItemDelegate(p_episodesListDelegate);

    setStatusTip(tr("集数栏 注意不要添加错误的集数"));

    connect(this, &episodesListWidget::clicked,
            this, &episodesListWidget::_doodle_episodes_emit);
}

episodesListWidget::~episodesListWidget()
{
}

void episodesListWidget::insertEpisodes()
{
    int raw = selectionModel()->currentIndex().row() + 1;
    p_episodesListModel->insertRow(raw, selectionModel()->currentIndex());
    //设置当前行的选择
    setCurrentIndex(p_episodesListModel->index(raw));
    edit(p_episodesListModel->index(raw));
}

void episodesListWidget::contextMenuEvent(QContextMenuEvent *event)
{
    p_eps_Menu = new QMenu(this);
    QAction *action = new QAction(this);

    connect(action, &QAction::triggered,
            this, &episodesListWidget::insertEpisodes);

    action->setText(tr("添加集数"));
    action->setStatusTip(tr("添加集数"));
    p_eps_Menu->addAction(action);

    p_eps_Menu->move(event->globalPos());
    p_eps_Menu->show();
}

void episodesListWidget::_doodle_episodes_emit(const QModelIndex &index)
{
    emit episodesEmit(p_episodesListModel->dataRaw(index));
}
DOODLE_NAMESPACE_E
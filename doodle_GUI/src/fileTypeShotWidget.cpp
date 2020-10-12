//
// Created by teXiao on 2020/10/12.
//

#include "fileTypeShotWidget.h"

#include "src/coreset.h"
#include "src/filetype.h"

#include <QLineEdit>
#include <QMessageBox>
#include <QMenu>

#include <QContextMenuEvent>
DOODLE_NAMESPACE_S
fileTypeShotModel::fileTypeShotModel(QObject *parent) : QAbstractListModel(parent) {}

int fileTypeShotModel::rowCount(const QModelIndex &parent) const {
  return p_type_ptr_list_.size();
}

QVariant fileTypeShotModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid()) return QVariant();
  if (index.row() >= p_type_ptr_list_.size()) return QVariant();

  if (role == Qt::DisplayRole || role == Qt::EditRole) {
    return p_type_ptr_list_[index.row()]->getFileType();
  } else {
    return QVariant();
  }
}

doCore::fileTypePtr fileTypeShotModel::daraRow(const QModelIndex &index) const {
  if (!index.isValid()) return nullptr;
  return p_type_ptr_list_[index.row()];
}
QVariant fileTypeShotModel::headerData(int section, Qt::Orientation Orientation, int role) const {
  return QAbstractItemModel::headerData(section, Orientation, role);
}
Qt::ItemFlags fileTypeShotModel::flags(const QModelIndex &index) const {
  if (!index.isValid()) return Qt::ItemIsEnabled;

  if (p_type_ptr_list_[index.row()]->isInsert())
    return QAbstractListModel::flags(index);
  else
    return Qt::ItemIsEnabled | Qt::ItemIsEditable | QAbstractListModel::flags(index);
}

void fileTypeShotModel::init(const doCore::fileClassPtr &file_class_ptr) {
  p_class_ptr_ = file_class_ptr;
  auto tmp_fileTypeList = doCore::fileType::getAll(file_class_ptr);
  clear();
  beginInsertRows(QModelIndex(), 0, tmp_fileTypeList.size());
  p_type_ptr_list_ = tmp_fileTypeList;
  endInsertRows();
}
void fileTypeShotModel::clear() {
  beginResetModel();
  p_type_ptr_list_.clear();
  endResetModel();
}
bool fileTypeShotModel::setData(const QModelIndex &index, const QVariant &value, int role) {
  if(index.isValid() && role == Qt::EditRole){
    bool isHas = false;
    for (const auto &item : p_type_ptr_list_) {
      if(item->isInsert() || value.toString() == item->getFileType()){
        isHas = true;
        break;
      }
    }

    if(!isHas){
      p_type_ptr_list_[index.row()]->setFileType(value.toString());
      p_type_ptr_list_[index.row()]->setFileClass(p_class_ptr_);
      p_type_ptr_list_[index.row()]->insert();
    } else{
      return false;
    }
  }
  return true;
}
bool fileTypeShotModel::insertRows(int position, int rows, const QModelIndex &index) {
  beginInsertRows(QModelIndex(),position , position + rows - 1);
  for (int row = 0; row < rows; ++row) {
    p_type_ptr_list_.insert(position, doCore::fileTypePtr(new doCore::fileType));
  }
  endInsertRows();
  return true;
}
bool fileTypeShotModel::removeRows(int position, int rows, const QModelIndex &index) {
  beginRemoveRows(QModelIndex(),position,position + rows -1 );
  for (int row = 0; row < rows; ++row) {
    p_type_ptr_list_.remove(position);
  }
  endRemoveRows();
  return true;
}

fileTypeShotModel::~fileTypeShotModel() = default;;

//-----------------------------------自定义委托---------------------------------------------//
fileTypeShotDelegate::fileTypeShotDelegate(QObject *parent) : QStyledItemDelegate(parent) {

}
QWidget *fileTypeShotDelegate::createEditor(QWidget *parent,
                                            const QStyleOptionViewItem &option_view_item,
                                            const QModelIndex &index) const {
  auto *fileTypeEdit = new QLineEdit(parent);
  return fileTypeEdit;
}
void fileTypeShotDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const {
  auto *edit = static_cast<QLineEdit *>(editor);
  edit->setText(doCore::coreSet::getCoreSet().getDepartment());
}
void fileTypeShotDelegate::setModelData(QWidget *editor,
                                        QAbstractItemModel *model, const QModelIndex &index) const {
  auto *shotedit = static_cast<QLineEdit *>(editor);

  auto data = shotedit->text();
  auto box = QMessageBox::information(static_cast<QWidget *>(this->parent()),
                                      tr("警告:"),
                                      tr("将种类 %1 提交到服务器").arg(data),
                                      QMessageBox::Yes | QMessageBox::Cancel);
  if (box == QMessageBox::Yes)
    model->setData(index, data, Qt::EditRole);
  else
    model->removeRow(index.row(), index);
}
void fileTypeShotDelegate::updateEditorGeometry(QWidget *editor,
                                                const QStyleOptionViewItem &option,
                                                const QModelIndex &index) const {
  editor->setGeometry(option.rect);
}
fileTypeShotDelegate::~fileTypeShotDelegate() = default;



//-----------------------------------自定义小部件---------------------------------------//

fileTypeShotWidget::fileTypeShotWidget(QWidget *parent)
    : QListView(parent),
    p_file_type_shot_model_(nullptr),
    p_file_type_shot_delegate_(nullptr),
    p_menu_(nullptr),
    p_file_class_ptr_(nullptr){
  p_file_type_shot_model_ = new fileTypeShotModel(this);
  p_file_type_shot_delegate_ = new fileTypeShotDelegate(this);

  setModel(p_file_type_shot_model_);
  setItemDelegate(p_file_type_shot_delegate_);

  setStatusTip("种类  使用右键添加");

  connect(this, &fileTypeShotWidget::clicked,
          this, &fileTypeShotWidget::_doodle_type_emit);

}

void fileTypeShotWidget::init(const doCore::fileClassPtr &file_class_ptr) {
  p_file_class_ptr_ = file_class_ptr;
  p_file_type_shot_model_->init(file_class_ptr);
}
void fileTypeShotWidget::insertFileType() {
  int row = selectionModel()->currentIndex().row() + 1;
  p_file_type_shot_model_->insertRow(row,QModelIndex());

  setCurrentIndex(p_file_type_shot_model_->index(row));
  edit(p_file_type_shot_model_->index(row));
}
void fileTypeShotWidget::_doodle_type_emit(const QModelIndex &index) {
  emit typeEmit(p_file_type_shot_model_->daraRow(index));
}
void fileTypeShotWidget::contextMenuEvent(QContextMenuEvent * event) {
  p_menu_ = new QMenu(this);
  if (p_file_class_ptr_){
    auto *action = new QAction(this);

    connect(action, &QAction::triggered,
            this, &fileTypeShotWidget::insertFileType);
    action->setText(tr("添加种类"));
    action->setToolTip(tr("添加镜头"));
    p_menu_->addAction(action);
  }
  p_menu_->move(event->globalPos());
  p_menu_->show();
}
void fileTypeShotWidget::clear() {
  p_file_type_shot_model_->clear();
};

fileTypeShotWidget::~fileTypeShotWidget() = default;

DOODLE_NAMESPACE_E






//
// Created by teXiao on 2020/10/16.
//

#include "fileTypeAssWidget.h"

#include "fileTypeAssModel.h"
#include "src/filetype.h"
#include "src/assClass.h"
#include <QMenu>

#include <QContextMenuEvent>
#include <QtWidgets/QMessageBox>

DOODLE_NAMESPACE_S
fileTypeAssDelegate::fileTypeAssDelegate(QObject *parent) : QStyledItemDelegate(parent) {

}
QWidget *fileTypeAssDelegate::createEditor(QWidget *parent,
                                           const QStyleOptionViewItem &option,
                                           const QModelIndex &index) const {
  auto * fileType = new QComboBox(parent);

  const auto modle = dynamic_cast<fileTypeAssModel *>(const_cast<QAbstractItemModel * >(index.model()));

  QStringList list;
  list << "sourceimages"
  << "scenes"
  << QString("%1_UE4").arg(modle->getAssTypePtr()->getAssClass())
  << "rig"
  << QString("%1_low").arg(modle->getAssTypePtr()->getAssClass());

  fileType->addItems(list);

  return fileType;
}
void fileTypeAssDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const {
  auto *edit = dynamic_cast<QComboBox *>(editor);

  edit->setCurrentIndex(0);
}
void fileTypeAssDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const {
  auto *edit = dynamic_cast<QComboBox *>(editor);

  auto value = edit->currentText();
  QMessageBox::StandardButton box = QMessageBox::information(dynamic_cast<QWidget *>(this->parent()),
                                                             tr("警告:"),
                                                             tr("将 %1 类型提交到服务器").arg(value),
                                                             QMessageBox::Yes | QMessageBox::Cancel);
  if (box == QMessageBox::Yes)
    model->setData(index, value, Qt::EditRole);
  else
    model->removeRow(index.row(), index);
}
void fileTypeAssDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                                               const QModelIndex &index) const {
  editor->setGeometry(option.rect);
}




fileTypeAssWidget::fileTypeAssWidget(QWidget *parent)
    : QListView(parent),
      p_type_ptr_list_(),
      p_menu_(nullptr),
      p_model_(nullptr),
      p_ass_type_ptr_(nullptr) {
  setItemDelegate(new fileTypeAssDelegate(this));
  connect(this,&fileTypeAssWidget::clicked,
          this,&fileTypeAssWidget::_doodle_type_emit);
}
void fileTypeAssWidget::setModel(QAbstractItemModel *model) {
  auto k_model = dynamic_cast<fileTypeAssModel *>(model);
  if (k_model)
    p_model_ = k_model;
  QAbstractItemView::setModel(model);
}
void fileTypeAssWidget::init(const doCore::assClassPtr &ass_type_ptr) {
  p_ass_type_ptr_ = ass_type_ptr;
  p_model_->init(ass_type_ptr);
}

void fileTypeAssWidget::clear() {
  p_ass_type_ptr_.reset();
  p_model_->clear();
}
void fileTypeAssWidget::inserttype() {
 int raw = selectionModel()->currentIndex().row();
 p_model_->insertRow(raw);
 setCurrentIndex(p_model_->index(raw));
 edit(p_model_->index(raw));
}
void fileTypeAssWidget::_doodle_type_emit(const QModelIndex &index) {
  emit filetypeEmited(index.data(Qt::UserRole).value<doCore::fileTypePtr>());
}
void fileTypeAssWidget::contextMenuEvent(QContextMenuEvent *event) {
  if(p_ass_type_ptr_ && !p_menu_){
    p_menu_ = new QMenu(this);
    auto add_ass_type = new QAction(p_menu_);
    add_ass_type->setText(tr("添加"));
    connect(add_ass_type,&QAction::triggered,
            this,&fileTypeAssWidget::inserttype);
    p_menu_->addAction(add_ass_type);
  }
  p_menu_->move(event->globalPos());
  p_menu_->show();
}

DOODLE_NAMESPACE_E

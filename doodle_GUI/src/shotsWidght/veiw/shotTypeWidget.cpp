//
// Created by teXiao on 2020/10/12.
//
#include < core_Cpp.h>
#include "src/shotsWidght/model/shotTypeModel.h"
#include "shotTypeWidget.h"

#include <QLineEdit>
#include <QMessageBox>
#include <QMenu>
#include <QContextMenuEvent>

DOODLE_NAMESPACE_S

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
  edit->setText(coreSet::getSet().getDepartmentQ());
}
void fileTypeShotDelegate::setModelData(QWidget *editor,
                                        QAbstractItemModel *model, const QModelIndex &index) const {
  auto *shotedit = static_cast<QLineEdit *>(editor);

  auto data = shotedit->text();
  auto box  = QMessageBox::information(static_cast<QWidget *>(this->parent()),
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

shotTypeWidget::shotTypeWidget(QWidget *parent)
    : QListView(parent),
      p_file_type_shot_model_(nullptr),
      p_file_type_shot_delegate_(nullptr),
      p_menu_(nullptr),
      p_file_class_ptr_(nullptr) {
  p_file_type_shot_delegate_ = new fileTypeShotDelegate(this);

  setItemDelegate(p_file_type_shot_delegate_);

  setStatusTip("种类  使用右键添加");

  connect(this, &shotTypeWidget::clicked,
          this, &shotTypeWidget::_doodle_chicked_emit);
}

void shotTypeWidget::insertFileType() {
  int row = selectionModel()->currentIndex().row() + 1;
  p_file_type_shot_model_->insertRow(row, QModelIndex());

  setCurrentIndex(p_file_type_shot_model_->index(row));
  edit(p_file_type_shot_model_->index(row));
}
void shotTypeWidget::_doodle_chicked_emit(const QModelIndex &index) {
  auto info = index.data(Qt::UserRole).value<shotType *>();
  if (info)
    doodleUseFilter(info->shared_from_this(), filterState::useFilter);
}
void shotTypeWidget::mousePressEvent(QMouseEvent *event) {
  QListView::mousePressEvent(event);
  if (!indexAt(event->pos()).isValid()) {
    clear();
    doodleUseFilter(nullptr, filterState::notFilter);
  }
}

void shotTypeWidget::clear() {
  selectionModel()->clearSelection();
  clearSelection();
  update();
}

shotTypeWidget::~shotTypeWidget() = default;

DOODLE_NAMESPACE_E

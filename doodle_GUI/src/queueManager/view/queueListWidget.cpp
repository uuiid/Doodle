#include "queueListWidget.h"
#include <core_Cpp.h>
#include <QtGui/QPainter>

#include <QtWidgets/QProgressBar>
#include <QtWidgets/QApplication>
// #include <QtWidgets/QStyleOptionProgressBar>
DOODLE_NAMESPACE_S
QueueListDelegate::QueueListDelegate(QWidget *parent)
    : QStyledItemDelegate(parent) {
}

void QueueListDelegate::paint(QPainter *painter,
                              const QStyleOptionViewItem &option,
                              const QModelIndex &index) const {
  if (!index.isValid()) return;

  auto k_queue = index.data(Qt::UserRole).value<queueDataPtr>();
  if (!k_queue) return;

  QStyleOptionProgressBar progress;
  progress.state         = option.state;
  progress.direction     = QApplication::layoutDirection();
  progress.rect          = option.rect;
  progress.fontMetrics   = QApplication::fontMetrics();
  progress.minimum       = 0;
  progress.maximum       = 100;
  progress.textAlignment = Qt::AlignCenter;
  progress.textVisible   = true;
  // progress.styleObject

  int progress_int  = k_queue->Progress();
  progress.progress = progress_int < 0 ? 0 : progress_int;
  progress.text     = QString::fromStdString(k_queue->Name());
  QApplication::style()->drawControl(QStyle::CE_ProgressBar, &progress, painter);
  //   progress.render(painter);
  //   painter->restore();
}

queueListWidget::queueListWidget(QWidget *parent)
    : QListView(parent) {
  setItemDelegate(new QueueListDelegate(this));
}

DOODLE_NAMESPACE_E

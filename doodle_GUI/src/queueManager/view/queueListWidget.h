#pragma once

#include <core_global.h>
#include <doodle_global.h>

#include <QtWidgets/QListView>

#include <QtWidgets/QStyledItemDelegate>

DOODLE_NAMESPACE_S
class QueueListDelegate : public QStyledItemDelegate {
  Q_OBJECT
 public:
  QueueListDelegate(QWidget *parent = nullptr);

  void paint(QPainter *painter,
             const QStyleOptionViewItem &option,
             const QModelIndex &index) const override;
};

class queueListWidget : public QListView {
  Q_OBJECT
 public:
  queueListWidget(QWidget *parent = nullptr);

 private:
};

DOODLE_NAMESPACE_E
#pragma once

#include <core_global.h>
#include <doodle_global.h>

#include <QtWidgets/QListView>

#include <QtWidgets/QStyledItemDelegate>
#include <QtGui/qsyntaxhighlighter.h>
DOODLE_NAMESPACE_S
class QueueListDelegate : public QStyledItemDelegate {
  Q_OBJECT
 public:
  explicit QueueListDelegate(QWidget *parent = nullptr);

  void paint(QPainter *painter,
             const QStyleOptionViewItem &option,
             const QModelIndex &index) const override;
};

class queueListWidget : public QListView {
  Q_OBJECT
 public:
  explicit queueListWidget(QWidget *parent = nullptr);

 private:
};

class QueueSyntaxHighlighter : public QSyntaxHighlighter {
  Q_OBJECT
 public:
  explicit QueueSyntaxHighlighter(QTextDocument  *parent = nullptr);

 protected:
  void highlightBlock(const QString &text) override;
};
DOODLE_NAMESPACE_E
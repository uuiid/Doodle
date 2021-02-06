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

  auto k_queue = index.data(Qt::UserRole).value<queueData *>();
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
  progress.styleObject   = option.styleObject;

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

QueueSyntaxHighlighter::QueueSyntaxHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent) {
}

void QueueSyntaxHighlighter::highlightBlock(const QString &text) {
  static QRegularExpression expressions{R"(write file|--> updata -->|--> down -->|create dir --> |update file|write file)"};
  static QRegularExpression expressionsnum{R"( : \d+\.\d+)"};

  auto iter    = expressions.globalMatch(text);
  auto iternum = expressionsnum.globalMatch(text);
  while (iter.hasNext()) {
    auto match = iter.next();
    setFormat(match.capturedStart(), match.capturedLength(), Qt::red);
  }
  while (iternum.hasNext()) {
    auto match = iternum.next();
    setFormat(match.capturedStart(), match.capturedLength(), Qt::green);
  }
}

DOODLE_NAMESPACE_E

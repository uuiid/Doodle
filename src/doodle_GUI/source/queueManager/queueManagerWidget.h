#pragma once

#include <core_global.h>
#include <doodle_global.h>

#include <QtWidgets/QWidget>
class QPlainTextEdit;

DOODLE_NAMESPACE_S
class queueManagerWidget : public QWidget {
  Q_OBJECT
 public:
  explicit queueManagerWidget(QWidget *parent = nullptr);

 Q_SIGNALS:
  void addTest(const QString &value);

 private:
  QPlainTextEdit *p_plain;
};

DOODLE_NAMESPACE_E

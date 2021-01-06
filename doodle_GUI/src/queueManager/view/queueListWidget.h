#pragma once

#include <core_global.h>
#include <doodle_global.h>

#include <QtWidgets/QListView>

DOODLE_NAMESPACE_S
class queueListWidget : public QListView {
  Q_OBJECT
 public:
  queueListWidget(QWidget *parent = nullptr);

 private:
};

DOODLE_NAMESPACE_E
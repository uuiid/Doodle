#pragma once

#include <core_global.h>
#include <doodle_global.h>

#include <QtCore/QAbstractListModel>

DOODLE_NAMESPACE_S
class queueManagerModel : public QAbstractListModel {
  Q_OBJECT
 public:
  explicit queueManagerModel(QObject *parent = nullptr);

 private:
};

DOODLE_NAMESPACE_E

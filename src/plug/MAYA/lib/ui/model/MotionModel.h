#pragma once

#include <MotionGlobal.h>

#include <QtCore/QAbstractListModel>

namespace doodle::motion::ui {
class MotionModel : public QAbstractListModel {
  Q_OBJECT
 private:
  
 public:
  MotionModel(QObject *parent = nullptr);
  ~MotionModel();
};


}  // namespace doodle::motion::ui
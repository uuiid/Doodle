#include <lib/ui/model/MotionModel.h>

namespace doodle::motion::ui {
MotionModel::MotionModel(QObject *parent) : QAbstractListModel(parent) {
}

MotionModel::~MotionModel() {
}

}  // namespace doodle::motion::ui
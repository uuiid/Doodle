#include "queueManager.h"
DOODLE_NAMESPACE_S

queueManager::queueManager() {
}
queueManager& queueManager::Get() noexcept {
  static queueManager install;
  return install;
}

void queueManager::append(const queueDataPtr& queue_data) {
  p_queueData.push_back(queue_data);
}

DOODLE_NAMESPACE_E

#include "queueManager.h"
#include <src/queueData/queueData.h>

DOODLE_NAMESPACE_S

boost::signals2::signal<void(queueDataPtr)> queueManager::appendEnd{};
boost::signals2::signal<void()> queueManager::removeData{};

queueManager::queueManager() {
}

queueManager& queueManager::Get() noexcept {
  static queueManager install;
  return install;
}

void queueManager::append(const queueDataPtr& queue_data) {
  queue_data->finishEd.connect([=]() { this->checkingQueue(); });
  p_queueData.push_back(queue_data);
  appendEnd(queue_data);
}

const std::vector<queueDataPtr>& queueManager::Queue() {
  return p_queueData;
}

void queueManager::checkingQueue() {
  auto it = std::remove_if(p_queueData.begin(), p_queueData.end(), [=](queueDataPtr& ptr) -> bool {
    return ptr->Progress() == 100;
  });

  if (it != p_queueData.end()) {
    auto it_index = std::distance(p_queueData.begin(), it);
    p_queueData.erase(it, p_queueData.end());
    removeData();
  }
}

DOODLE_NAMESPACE_E

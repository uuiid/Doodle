#pragma once

#include <core_global.h>

DOODLE_NAMESPACE_S
class CORE_API queueManager {
 public:
  DOODLE_DISABLE_COPY(queueManager)

  [[nodiscard]] static queueManager& Get() noexcept;

  void append(const queueDataPtr& queue_data);

 private:
  queueManager();

 private:
  std::vector<queueDataPtr> p_queueData;
};

DOODLE_NAMESPACE_E
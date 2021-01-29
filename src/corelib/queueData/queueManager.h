#pragma once

#include <corelib/core_global.h>
#include <boost/signals2.hpp>

DOODLE_NAMESPACE_S
class CORE_API queueManager {
 public:
  DOODLE_DISABLE_COPY(queueManager)

  static boost::signals2::signal<void(queueDataPtr)> appendEnd;
  static boost::signals2::signal<void()> removeData;

  [[nodiscard]] static queueManager& Get() noexcept;

  void append(const queueDataPtr& queue_data);

  const std::vector<queueDataPtr>& Queue();
  void checkingQueue();

 private:
  queueManager();

 private:
  std::vector<queueDataPtr> p_queueData;
};

DOODLE_NAMESPACE_E
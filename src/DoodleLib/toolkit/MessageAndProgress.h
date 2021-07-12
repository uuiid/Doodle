#pragma once

#include <DoodleLib/DoodleLib_fwd.h>

namespace doodle {
// BEGIN_DECLARE_EVENT_TYPES()
// DECLARE_EVENT_TYPE(DOODLE_THREAD, -101)
// END_DECLARE_EVENT_TYPES()
class MessageAndProgress {
  std::string p_message;


 public:
  MessageAndProgress();

  template <typename T>
  void createProgress(std::shared_ptr<T> value);
};

template <typename T>
void MessageAndProgress::createProgress(std::shared_ptr<T> value) {
  //连接进度
  value->sig_progress.connect([this](int i) {
  });
  //连接消息
  value->sig_message_result.connect([this](const std::string& message) {
  });
  //连接完成信号
  value->sig_finished.connect([this] {
  });
}
}  // namespace doodle

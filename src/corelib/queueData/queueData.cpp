#include "queueData.h"

#include <corelib/queueData/queueManager.h>

DOODLE_NAMESPACE_S

queueData::queueData(futureB& f)
    : p_fu(std::move(f)),
      p_name("Queue"),
      p_progress(0),
      p_string() {
}

void queueData::submit() {
  queueManager::Get().append(shared_from_this());
}

void queueData::appendInfo(const std::string& str) noexcept {
  p_string.append(str);
  InfoChanged(str);
}

const std::string& queueData::Info() const noexcept {
  return p_string;
}

const int& queueData::Progress() const noexcept {
  static int tmp100{100};
  static int tmp99{99};
  if (p_fu.valid()) {
    if (p_fu.wait_for(std::chrono::milliseconds(1)) == std::future_status::ready) {
      return tmp100;
    } else {
      return p_progress >= tmp99 ? tmp99 : p_progress;
    }
  } else {
    return tmp100;
  }
}

void queueData::setProgress(const int value) noexcept {
  p_progress += value;
  ProgressChanged();
}

const queueData::futureB& queueData::Future() const noexcept {
  return p_fu;
}

void queueData::setName(const std::string& name) noexcept {
  p_name = name;
}

const std::string& queueData::Name() const noexcept {
  return p_name;
}

DOODLE_NAMESPACE_E
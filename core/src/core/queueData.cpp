#include "queueData.h"

DOODLE_NAMESPACE_S
queueData::queueData(futureB&& f)
    : p_fu(std::move(f)),
      p_name("Queue"),
      p_progress(0),
      p_string() {
}

void queueData::appendInfo(const std::string& str) noexcept {
  p_string.append(str);
}

const std::string& queueData::Info() const noexcept {
  return p_string;
}

const int& queueData::progress() const noexcept {
  return p_progress;
}

const queueData::futureB& queueData::future() const noexcept {
  return p_fu;
}

void queueData::setName(const std::string& name) noexcept {
  p_name = name;
}

const std::string& queueData::Name() const noexcept {
  return p_name;
}

DOODLE_NAMESPACE_E
#include <lib/kernel/MotionSetting.h>

namespace doodle::motion::kernel {

MotionSetting* MotionSetting::p_setting = nullptr;

MotionSetting& MotionSetting::Get() {
  if (!p_setting)
    Create();
  return *p_setting;
}

MotionSetting* MotionSetting::Create() {
  if (!p_setting)
    p_setting = new MotionSetting();
  else
    return p_setting;
}

const FSys::path& MotionSetting::MotionLibRoot() const noexcept {
  return p_motion_path;
}

void MotionSetting::setMotionLibRoot(const FSys::path& MotionLibRoot) noexcept {
  p_motion_path = MotionLibRoot;
}

const std::string& MotionSetting::User() const noexcept {
  return p_user_name;
}

void MotionSetting::setUser(const std::string& User) noexcept {
  p_user_name = User;
}

MotionSetting::MotionSetting()
    : p_motion_path() {
}

}  // namespace doodle::motion::kernel
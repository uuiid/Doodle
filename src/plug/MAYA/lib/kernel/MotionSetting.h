#pragma once
#include <lib/MotionGlobal.h>
#include <filesystem>
#include <map>
namespace doodle::motion::kernel {
class MotionSetting {
 public:
  MotionSetting(const MotionSetting&) = delete;
  MotionSetting& operator=(const MotionSetting&) = delete;

  static MotionSetting& Get();
  static MotionSetting* Create();
  const FSys::path& MotionLibRoot() const noexcept;
  void setMotionLibRoot(const FSys::path& MotionLibRoot) noexcept;

  const std::string& User() const noexcept;
  void setUser(const std::string& User) noexcept;

 private:
  MotionSetting();

 private:
  FSys::path p_motion_path;
  std::string p_user_name;
  static MotionSetting* p_setting;
};
}  // namespace doodle::motion::kernel
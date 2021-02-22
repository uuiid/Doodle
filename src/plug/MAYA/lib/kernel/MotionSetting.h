#pragma once
#include <lib/MotionGlobal.h>
#include <filesystem>
#include <map>

#include <nlohmann/json_fwd.hpp>
namespace doodle::motion::kernel {
class MotionSetting {
 public:
  MotionSetting(const MotionSetting&) = delete;
  MotionSetting& operator=(const MotionSetting&) = delete;

  static MotionSetting& Get();
  const FSys::path& MotionLibRoot() const noexcept;
  void setMotionLibRoot(const FSys::path& MotionLibRoot) noexcept;

  const std::string& User() const noexcept;
  void setUser(const std::string& User) noexcept;

  void save();

 private:
  MotionSetting();

  void from_json(const nlohmann::json& j);
  nlohmann::json to_json();

 private:
  FSys::path p_setting_path;

  FSys::path p_motion_path;
  std::string p_user_name;
};
}  // namespace doodle::motion::kernel
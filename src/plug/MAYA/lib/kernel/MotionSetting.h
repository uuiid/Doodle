#pragma once
#include <lib/MotionGlobal.h>
#include <lib/kernel/BoostUuidWarp.h>

#include <boost/signals2.hpp>
#include <nlohmann/json_fwd.hpp>
namespace doodle::motion::kernel {
class MotionSetting {
 public:
  MotionSetting(const MotionSetting&) = delete;
  MotionSetting& operator=(const MotionSetting&) = delete;

  static MotionSetting& Get();
  bool openMotionProject();
  const FSys::path& MotionLibRoot() const noexcept;
  void setMotionLibRoot(const FSys::path& MotionLibRoot) noexcept;

  const std::string& User() const noexcept;
  void setUser(const std::string& User) noexcept;

  const std::string& MotionName() const noexcept;
  void setMotionName(const std::string& MotionName) noexcept;

  void save();

  const boost::uuids::uuid random_generator();

  boost::signals2::signal<void()> InitProject;

 private:
  MotionSetting();
  ~MotionSetting() = default;

  void createMotionProject();

  void from_json(const nlohmann::json& j);
  nlohmann::json to_json();

 private:
  FSys::path p_setting_path;

  FSys::path p_motion_path;
  std::string p_user_name;
  std::string p_motion_name;

  boost::uuids::random_generator p_uuid;
  static std::string ConfigName;
};
}  // namespace doodle::motion::kernel

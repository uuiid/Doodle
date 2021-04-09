#pragma once
#include <corelib/core_global.h>

namespace doodle {
class CORE_API Shot {
  int64_t p_shot;
  std::string p_shot_ab;

 public:
  Shot();
  Shot(int64_t in_shot, std::string in_shot_ab = {});

  const int64_t& Shot_() const noexcept;
  void setShot_(const int64_t& Shot_);

  const std::string& ShotAb() const noexcept;
  void setShotAb(const std::string& ShotAb) noexcept;

  std::string str() const;
};
}  // namespace doodle
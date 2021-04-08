#pragma once

#include <corelib/core_global.h>

namespace doodle {
class Ue4Setting {
  std::string ue4_version;
  FSys::path ue4_path;
  std::int32_t shot_start;
  std::int32_t shot_end;

  Ue4Setting();

 public:
  DOODLE_DISABLE_COPY(Ue4Setting);

  static Ue4Setting& Get();

  const std::string& Version() const noexcept;
  void setVersion(const std::string& Version) noexcept;

  const FSys::path& Path() const noexcept;
  void setPath(const FSys::path& Path) noexcept;

  const std::int32_t& ShotStart() const noexcept;
  void setShotStart(const std::int32_t& ShotStart) noexcept;

  const std::int32_t& ShotEnd() const noexcept;
  void setShotEnd(const std::int32_t& ShotEnd) noexcept;
};

}  // namespace doodle
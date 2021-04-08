#include <corelib/core/Ue4Setting.h>

namespace doodle {
Ue4Setting::Ue4Setting() {
}

Ue4Setting& Ue4Setting::Get() {
  static Ue4Setting install;
  return install;
}

const std::string& Ue4Setting::Version() const noexcept {
  return ue4_version;
}

void Ue4Setting::setVersion(const std::string& Version) noexcept {
  ue4_version = Version;
}

const FSys::path& Ue4Setting::Path() const noexcept {
  return ue4_path;
}

void Ue4Setting::setPath(const FSys::path& Path) noexcept {
  ue4_path = Path;
}

const std::int32_t& Ue4Setting::ShotStart() const noexcept {
  return shot_start;
}

void Ue4Setting::setShotStart(const std::int32_t& ShotStart) noexcept {
  shot_start = ShotStart;
}

const std::int32_t& Ue4Setting::ShotEnd() const noexcept {
  return shot_end;
}

void Ue4Setting::setShotEnd(const std::int32_t& ShotEnd) noexcept {
  shot_end = ShotEnd;
}
}  // namespace doodle
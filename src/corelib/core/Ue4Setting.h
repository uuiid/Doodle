#pragma once

#include <corelib/core_global.h>
#include <cereal/cereal.hpp>
#include <cereal/types/common.hpp>
#include <cereal/types/string.hpp>

namespace doodle {
class CORE_API Ue4Setting {
  FSys::path ue4_path;
  std::string ue4_version;
  std::int32_t shot_start;
  std::int32_t shot_end;

  Ue4Setting();

 public:
  DOODLE_DISABLE_COPY(Ue4Setting);

  static Ue4Setting& Get();

  const std::string& Version() const noexcept;
  void setVersion(const std::string& Version) noexcept;

  bool hasPath() const;
  const FSys::path& Path() const noexcept;
  void setPath(const FSys::path& Path) noexcept;

  const std::int32_t& ShotStart() const noexcept;
  void setShotStart(const std::int32_t& ShotStart) noexcept;

  const std::int32_t& ShotEnd() const noexcept;
  void setShotEnd(const std::int32_t& ShotEnd) noexcept;

  void testValue();

 private:
  friend class cereal::access;

  template <class Archive>
  void serialize(Archive& ar, std::uint32_t const version);
};

template <class Archive>
void Ue4Setting::serialize(Archive& ar, std::uint32_t const version) {
  if (version == 1)
    ar(
        cereal::make_nvp("ue4_path", ue4_path),
        cereal::make_nvp("ue4_version", ue4_version),
        cereal::make_nvp("shot_start", shot_start),
        cereal::make_nvp("shot_end", shot_end));
}

}  // namespace doodle

CEREAL_CLASS_VERSION(doodle::Ue4Setting, 1);

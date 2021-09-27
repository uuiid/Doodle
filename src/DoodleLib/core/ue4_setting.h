#pragma once

#include <DoodleLib/DoodleLib_fwd.h>

namespace doodle {
class DOODLELIB_API ue4_setting :public details::no_copy{
  FSys::path ue4_path;
  std::string ue4_version;
  std::int32_t shot_start;
  std::int32_t shot_end;

  ue4_setting();

 public:

  static ue4_setting& Get();

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
  friend class boost::serialization::access;

  template <class Archive>
  void serialize(Archive& ar, std::uint32_t const version);
};

template <class Archive>
void ue4_setting::serialize(Archive& ar, std::uint32_t const version) {
  if (version == 1)
    ar&
        boost::serialization::make_nvp("ue4_path", ue4_path)&
        boost::serialization::make_nvp("ue4_version", ue4_version)&
        boost::serialization::make_nvp("shot_start", shot_start)&
        boost::serialization::make_nvp("shot_end", shot_end);
}

}  // namespace doodle

BOOST_CLASS_VERSION(doodle::ue4_setting, 1);
BOOST_CLASS_EXPORT_KEY(doodle::ue4_setting);

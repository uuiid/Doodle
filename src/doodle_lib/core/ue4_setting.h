#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle {
class DOODLELIB_API ue4_setting : public details::no_copy {
  FSys::path ue4_path;
  std::string ue4_version;
  std::int32_t shot_start;
  std::int32_t shot_end;

  ue4_setting();

 public:
  static ue4_setting& Get();

  const std::string& get_version() const noexcept;
  void set_version(const std::string& Version) noexcept;

  bool has_path() const;
  const FSys::path& get_path() const noexcept;
  void set_path(const FSys::path& Path) noexcept;

  void test_value();

 private:
  friend class boost::serialization::access;

  template <class Archive>
  void serialize(Archive& ar, std::uint32_t const version);
  friend void to_json(nlohmann::json& j, const ue4_setting& p);
  friend void from_json(const nlohmann::json& j, ue4_setting& p);
};

void to_json(nlohmann::json& j, const ue4_setting& p);
void from_json(const nlohmann::json& j, ue4_setting& p);

template <class Archive>
void ue4_setting::serialize(Archive& ar, std::uint32_t const version) {
  if (version == 1)
    ar&
            boost::serialization::make_nvp("ue4_path", ue4_path) &
        boost::serialization::make_nvp("ue4_version", ue4_version) &
        boost::serialization::make_nvp("shot_start", shot_start) &
        boost::serialization::make_nvp("shot_end", shot_end);
}

}  // namespace doodle


#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/metadata/metadata.h>

namespace doodle {
/**
 * 项目信息类
 */
class DOODLELIB_API project {
  std::string p_name;
  std::string p_en_str;
  std::string p_shor_str;
  FSys::path p_path;
  FSys::path p_sim_path;
  void init_name();

 public:
  project();
  explicit project(FSys::path in_path, std::string in_name = {});

  [[nodiscard]] const std::string& get_name() const;
  void set_name(const std::string& Name) noexcept;

  [[nodiscard]] const FSys::path& get_path() const noexcept;
  void set_path(const FSys::path& Path);

  [[nodiscard]] std::string str() const;
  [[nodiscard]] std::string show_str() const;

  [[nodiscard]] std::string short_str() const;

  [[nodiscard]] const FSys::path& get_vfx_cloth_sim_path() const;
  void set_vfx_cloth_sim_path(const FSys::path& in_path);

  virtual void attribute_widget(const attribute_factory_ptr& in_factoryPtr);

  bool operator<(const project& in_rhs) const;
  bool operator>(const project& in_rhs) const;
  bool operator<=(const project& in_rhs) const;
  bool operator>=(const project& in_rhs) const;
  bool operator==(const project& in_rhs) const;
  bool operator!=(const project& in_rhs) const;

 private:
  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive& ar, std::uint32_t const version);
};

template <class Archive>
void project::serialize(Archive& ar, std::uint32_t const version) {
  if (version == 2) {
    ar& BOOST_SERIALIZATION_NVP(p_name);
    ar& BOOST_SERIALIZATION_NVP(p_path);
  }
  if (version == 3) {
    ar& BOOST_SERIALIZATION_NVP(p_name);
    ar& BOOST_SERIALIZATION_NVP(p_path);
    ar& BOOST_SERIALIZATION_NVP(p_sim_path);
  }
  init_name();
}

}  // namespace doodle
BOOST_CLASS_VERSION(doodle::project, 3);
BOOST_CLASS_EXPORT_KEY(doodle::project)

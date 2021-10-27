#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/metadata/metadata.h>

namespace doodle {
/**
 * 项目信息类
 */
class DOODLELIB_API project : public metadata {
  std::string p_name;
  std::string p_en_str;
  std::string p_shor_str;
  FSys::path p_path;

  void init();

 public:
  project();
  explicit project(FSys::path in_path, std::string in_name = {});
  DOODLE_MOVE(project)
  const std::string& get_name() const;
  void set_name(const std::string& Name) noexcept;

  [[nodiscard]] const FSys::path& get_path() const noexcept;
  void set_path(const FSys::path& Path);

  [[nodiscard]] std::string str() const override;
  [[nodiscard]] std::string show_str() const override;

  [[nodiscard]] std::string short_str() const;

  static std::string get_config_file_name();
  static std::string get_config_file_folder();

  virtual void attribute_widget(const attribute_factory_ptr& in_factoryPtr) override;

  bool operator<(const project& in_rhs) const;
  bool operator>(const project& in_rhs) const;
  bool operator<=(const project& in_rhs) const;
  bool operator>=(const project& in_rhs) const;

 private:
  [[nodiscard]] FSys::path DBRoot() const;

  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive& ar, std::uint32_t const version);
};

template <class Archive>
void project::serialize(Archive& ar, std::uint32_t const version) {
  if (version == 1)
    ar&
            boost::serialization::make_nvp("name", p_name) &
        boost::serialization::make_nvp("path", p_path);
  if (version == 2)
    ar&
            boost::serialization::make_nvp("metadata", boost::serialization::base_object<metadata>(*this)) &
        boost::serialization::make_nvp("name", p_name) &
        boost::serialization::make_nvp("path", p_path);
  init();
}

}  // namespace doodle
BOOST_CLASS_VERSION(doodle::project, 2);
BOOST_CLASS_EXPORT_KEY(doodle::project)

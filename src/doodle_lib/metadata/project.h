#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/metadata/metadata.h>

namespace doodle {
/**
 * 项目信息类
 */
class DOODLELIB_API project {
 public:
  class DOODLELIB_API cloth_config {
   public:
    cloth_config();
    FSys::path vfx_cloth_sim_path;
    bool simple_subsampling;
    std::float_t frame_samples;
    std::float_t time_scale;
    std::float_t length_scale;
    friend class boost::serialization::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version) {
      if (version == 1) {
        ar& BOOST_SERIALIZATION_NVP(vfx_cloth_sim_path);
        ar& BOOST_SERIALIZATION_NVP(simple_subsampling);
        ar& BOOST_SERIALIZATION_NVP(frame_samples);
        ar& BOOST_SERIALIZATION_NVP(time_scale);
        ar& BOOST_SERIALIZATION_NVP(length_scale);
      }
    }
  };
  using cloth_config_ptr = std::shared_ptr<cloth_config>;

 private:
  std::string p_name;
  std::string p_en_str;
  std::string p_shor_str;
  FSys::path p_path;
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

  cloth_config& get_vfx_cloth_config() const;

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
  void save(Archive& ar, const std::uint32_t version) const {
    ar& BOOST_SERIALIZATION_NVP(p_name);
    ar& BOOST_SERIALIZATION_NVP(p_path);
  }

  template <class Archive>
  void load(Archive& ar, const std::uint32_t version) {
    if (version == 2) {
      ar& BOOST_SERIALIZATION_NVP(p_name);
      ar& BOOST_SERIALIZATION_NVP(p_path);
    }
    init_name();
  }
  BOOST_SERIALIZATION_SPLIT_MEMBER()
};

}  // namespace doodle
BOOST_CLASS_VERSION(doodle::project, 2);
BOOST_CLASS_EXPORT_KEY(doodle::project)
BOOST_CLASS_VERSION(doodle::project::cloth_config, 1);
BOOST_CLASS_EXPORT_KEY(doodle::project::cloth_config)

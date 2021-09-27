#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Metadata/metadata.h>

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

  const std::string& getName() const;
  void setName(const std::string& Name) noexcept;

  [[nodiscard]] const FSys::path& getPath() const noexcept;
  void setPath(const FSys::path& Path);

  [[nodiscard]] std::string str() const override;
  [[nodiscard]] std::string showStr() const override;

  [[nodiscard]] std::string shortStr() const;

  static std::string getConfigFileName();
  static std::string getConfigFileFolder();

  virtual void create_menu(const attribute_factory_ptr& in_factoryPtr) override;

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
            boost::serialization::make_nvp("Metadata", boost::serialization::base_object<metadata>(*this)) &
        boost::serialization::make_nvp("name", p_name) &
        boost::serialization::make_nvp("path", p_path);
  init();
}

}  // namespace doodle
BOOST_CLASS_VERSION(doodle::project, 2);
BOOST_CLASS_EXPORT_KEY(doodle::project)

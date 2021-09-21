#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Metadata/Metadata.h>

#include <cereal/cereal.hpp>
#include <cereal/types/common.hpp>
#include <cereal/types/string.hpp>
namespace doodle {
/**
 * 项目信息类
 */
class DOODLELIB_API Project : public Metadata {
  std::string p_name;
  std::string p_en_str;
  std::string p_shor_str;
  FSys::path p_path;

  void init();

 public:
  Project();
  explicit Project(FSys::path in_path, std::string in_name = {});

  const std::string& getName() const;
  void setName(const std::string& Name) noexcept;

  [[nodiscard]] const FSys::path& getPath() const noexcept;
  void setPath(const FSys::path& Path);

  [[nodiscard]] std::string str() const override;
  [[nodiscard]] std::string showStr() const override;

  [[nodiscard]] std::string shortStr() const;

  static std::string getConfigFileName();
  static std::string getConfigFileFolder();

  virtual void create_menu(const menu_factory_ptr& in_factoryPtr) override;

  bool operator<(const Project& in_rhs) const;
  bool operator>(const Project& in_rhs) const;
  bool operator<=(const Project& in_rhs) const;
  bool operator>=(const Project& in_rhs) const;

 private:
  [[nodiscard]] FSys::path DBRoot() const;

  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive& ar, std::uint32_t const version);
};

template <class Archive>
void Project::serialize(Archive& ar, std::uint32_t const version) {
  if (version == 1)
    ar&
            boost::serialization::make_nvp("name", p_name) &
        boost::serialization::make_nvp("path", p_path);
  if (version == 2)
    ar&
            boost::serialization::make_nvp("Metadata", boost::serialization::base_object<Metadata>(*this)) &
        boost::serialization::make_nvp("name", p_name) &
        boost::serialization::make_nvp("path", p_path);
  init();
}

}  // namespace doodle
BOOST_CLASS_VERSION(doodle::Project, 2);

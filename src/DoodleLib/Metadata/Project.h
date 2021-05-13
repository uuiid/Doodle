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
  FSys::path p_path;

 public:
  Project();
  explicit Project(FSys::path in_path, std::string in_name = {});

  /// 属性编辑
  /// \return 项目名称属性
  [[nodiscard]] const std::string& Name() const noexcept;
  void setName(const std::string& Name) noexcept;

  [[nodiscard]] const FSys::path& Path() const noexcept;
  void setPath(const FSys::path& Path);

  [[nodiscard]] std::string str() const override;
  [[nodiscard]] std::string ShowStr() const override;

  [[nodiscard]] std::string ShortStr() const;

  virtual void SetPParent(const std::shared_ptr<Metadata>& in_parent) override;
  void load(const MetadataFactoryPtr& in_factory) override;
  void save(const MetadataFactoryPtr& in_factory) override;

  static std::string getConfigFileName();
  static std::string getConfigFileFolder();
  bool operator<(const Project& in_rhs) const;
  bool operator>(const Project& in_rhs) const;
  bool operator<=(const Project& in_rhs) const;
  bool operator>=(const Project& in_rhs) const;

 protected:
  virtual bool sort(const Metadata& in_rhs) const override;
  void modifyParent(const std::shared_ptr<Metadata>& in_old_parent) override;

 private:
  [[nodiscard]] FSys::path DBRoot() const;

  friend class cereal::access;
  template <class Archive>
  void serialize(Archive& ar, std::uint32_t const version);
};

template <class Archive>
void Project::serialize(Archive& ar, std::uint32_t const version) {
  if (version == 1)
    ar(
        cereal::make_nvp("name", p_name),
        cereal::make_nvp("path", p_path));
  if (version == 2)
    ar(
        cereal::make_nvp("Metadata", cereal::base_class<Metadata>(this)),
        cereal::make_nvp("name", p_name),
        cereal::make_nvp("path", p_path));
}

}  // namespace doodle
CEREAL_REGISTER_TYPE(doodle::Project)
CEREAL_CLASS_VERSION(doodle::Project, 2);

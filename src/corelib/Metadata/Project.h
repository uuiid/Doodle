#pragma once
#include <corelib/core_global.h>

#include <corelib/Metadata/Metadata.h>

#include <cereal/cereal.hpp>
#include <cereal/types/common.hpp>
#include <cereal/types/string.hpp>
namespace doodle {
/**
 * 项目信息类
 */
class CORE_API Project : public Metadata {
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

  [[nodiscard]] FSys::path DBRoot() const;

  [[nodiscard]] std::string str() const override;
  [[nodiscard]] std::string ShortStr() const;
  [[nodiscard]] std::string ShowStr() const override;

  void makeProject() const;
  [[nodiscard]] bool ChickProject() const;
  void ReadProject();

  static std::string getConfigFileName();
  static std::string getConfigFileFolder();

 private:
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
}

}  // namespace doodle
CEREAL_CLASS_VERSION(doodle::Project, 1);

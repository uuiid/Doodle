#pragma once

#include <corelib/core_global.h>
#include <corelib/libWarp/BoostUuidWarp.h>
#include <corelib/core/Ue4Setting.h>
#include <corelib/core/MetadataSet.h>
#include <corelib/Metadata/Project.h>

#include <cereal/cereal.hpp>
#include <cereal/types/common.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/base_class.hpp>
#include <cereal/types/memory.hpp>

#include <magic_enum.hpp>
#include <boost/filesystem.hpp>

DOODLE_NAMESPACE_S

enum class Department {
  None_,
  Executive,
  Light,
  VFX,
  modle,
  rig,
  Anm,
  direct,
  paint
};

/*
 *全局静态设置类
 */

class CORE_API coreSet {
 public:
  static coreSet &getSet();

  DOODLE_DISABLE_COPY(coreSet)
  //初始化函数
  void init();
  void reInit();

  void findMaya();

  //获得运行程序目录
  static FSys::path program_location();
  static FSys::path program_location(const FSys::path &path);

  [[nodiscard]] bool hasMaya() const noexcept;
  [[nodiscard]] const FSys::path &MayaPath() const noexcept;
  void setMayaPath(const FSys::path &in_MayaPath) noexcept;

  // user设置
  [[nodiscard]] std::string getUser() const;
  [[nodiscard]] std::string getUser_en() const;
  void setUser(const std::string &value);

  //部门设置
  [[nodiscard]] std::string getDepartment() const;
  [[nodiscard]] const Department &getDepartmentEnum() const;
  void setDepartment(const std::string &value);

  //缓存路径
  [[nodiscard]] FSys::path getCacheRoot() const;
  [[nodiscard]] FSys::path getCacheRoot(const FSys::path &path) const;

  // doc路径
  [[nodiscard]] FSys::path getDoc() const;

  [[nodiscard]] Ue4Setting &gettUe4Setting() const { return ue4_setting; };
  [[nodiscard]] MetadataSet &GetMetadataSet() const { return p_matadata_setting_; };

  void writeDoodleLocalSet();


  boost::uuids::uuid getUUID();
  std::string getUUIDStr() ;
  static void hideFolder(const FSys::path& path);

 private:
  static std::string toIpPath(const std::string &path);
  //私有化构造函数
  coreSet();
  //获得缓存磁盘路径
  void getCacheDiskPath();
  //获得本地的有限设置
  void getSetting();

  static std::string configFileName();

 private:
  boost::uuids::random_generator p_uuid_gen;

  //用户名称
  std::string user;
  //部门
  Department department;

  FSys::path cacheRoot;
  FSys::path doc;

  Ue4Setting &ue4_setting;
  MetadataSet &p_matadata_setting_;

  std::vector<std::shared_ptr<Project>> p_project_list;
  std::shared_ptr<Project> p_project;
  FSys::path p_mayaPath;

  //这里是序列化的代码
  friend class cereal::access;
  template <class Archive>
  void serialize(Archive &ar, std::uint32_t const version);

};

template <class Archive>
void coreSet::serialize(Archive &ar, std::uint32_t const version) {
  if(version == 4)
  ar(
      cereal::make_nvp("user", user),
      cereal::make_nvp("department", department),
      cereal::make_nvp("ue4_setting", ue4_setting),
      cereal::make_nvp("matadata_setting", p_matadata_setting_),
      cereal::make_nvp("maya_Path", p_mayaPath));
}


DOODLE_NAMESPACE_E
namespace cereal {
template <class Archive>
std::string save_minimal(Archive const &, doodle::Department const &department) {
  return std::string{magic_enum::enum_name(department)};
}
template <class Archive>
void load_minimal(Archive const &, doodle::Department &department, std::string const &value) {
  department = magic_enum::enum_cast<doodle::Department>(value).value_or(doodle::Department::VFX);
};
}  // namespace cereal
CEREAL_CLASS_VERSION(doodle::coreSet, 4);

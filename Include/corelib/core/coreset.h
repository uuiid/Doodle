#pragma once

#include <corelib/core_global.h>
#include <boost/filesystem.hpp>
#include <corelib/libWarp/BoostUuidWarp.h>
#include <corelib/core/Ue4Setting.h>

#include <cereal/cereal.hpp>
#include <cereal/types/common.hpp>
#include <cereal/types/string.hpp>
#include <magic_enum.hpp>

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

struct synPath_struct {
  fileSys::path local;
  fileSys::path server;
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

  void appendEnvironment() const;

  //获得运行程序目录
  static fileSys::path program_location();
  static fileSys::path program_location(const fileSys::path &path);

  // user设置
  [[nodiscard]] dstring getUser() const;
  [[nodiscard]] dstring getUser_en() const;
  void setUser(const dstring &value);

  //部门设置
  [[nodiscard]] dstring getDepartment() const;
  void setDepartment(const dstring &value);

  //缓存路径
  [[nodiscard]] fileSys::path getCacheRoot() const;

  // doc路径
  [[nodiscard]] fileSys::path getDoc() const;

  Ue4Setting &gettUe4Setting() const { return ue4_setting; };

  void writeDoodleLocalSet();

  static dstring toIpPath(const dstring &path);

  boost::uuids::uuid getUUID();

 private:
  //私有化构造函数
  coreSet();
  //获得缓存磁盘路径
  void getCacheDiskPath();
  //获得本地的有限设置
  void getSetting();

 private:
  const static dstring settingFileName;
  boost::uuids::random_generator p_uuid_gen;

  //用户名称
  std::string user;
  //部门
  Department department;

  fileSys::path cacheRoot;
  fileSys::path doc;

  Ue4Setting &ue4_setting;

  friend class cereal::access;

  template <class Archive>
  void save(Archive &ar, std::uint32_t const version) const;

  template <class Archive>
  void load(Archive &ar, std::uint32_t const version);
};

template <class Archive>
void coreSet::save(Archive &ar, std::uint32_t const version) const {
  ar(
      cereal::make_nvp("user", user),
      cereal::make_nvp("department", std::string{magic_enum::enum_name(department)}),
      cereal::make_nvp("ue4_setting", ue4_setting));
}

template <class Archive>
void coreSet::load(Archive &ar, std::uint32_t const version) {
  std::string deps{};
  ar(
      user,
      deps,
      ue4_setting);
  this->department = magic_enum::enum_cast<doodle::Department>(deps).value_or(doodle::Department::VFX);
}

DOODLE_NAMESPACE_E

CEREAL_CLASS_VERSION(doodle::coreSet, 1);

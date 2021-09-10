#pragma once

#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Metadata/Project.h>
#include <DoodleLib/core/Ue4Setting.h>
#include <DoodleLib/core/observable_container.h>
#include <DoodleLib/libWarp/BoostUuidWarp.h>

#include <boost/filesystem.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/base_class.hpp>
#include <cereal/types/common.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <magic_enum.hpp>

#include <nlohmann/json_fwd.hpp>
namespace doodle {

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

class DOODLELIB_API CoreSet : public details::no_copy {
 public:
  static CoreSet &getSet();

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
  void setCacheRoot(const FSys::path &path);

  FSys::path getDataRoot() const;
  void setDataRoot(const FSys::path &in_path);

  // doc路径
  [[nodiscard]] FSys::path getDoc() const;

  [[nodiscard]] Ue4Setting &gettUe4Setting() const { return p_ue4_setting; };

  [[nodiscard]] int getSqlPort() const;
  void setSqlPort(int in_sqlPort);
  [[nodiscard]] const std::string &getSqlHost() const;
  void setSqlHost(const std::string &in_sqlHost);
  [[nodiscard]] const std::string &getSqlUser() const;
  void setSqlUser(const std::string &in_sqlUser);
  [[nodiscard]] const std::string &getSqlPassword() const;
  void setSqlPassword(const std::string &in_sqlPassword);
  [[nodiscard]] int getMetaRpcPort() const;
  void setMetaRpcPort(int in_metaRpcPort);
  [[nodiscard]] int getFileRpcPort() const;
  void setFileRpcPort(int in_fileRpcPort);

  void writeDoodleLocalSet();

  boost::uuids::uuid getUUID();
  std::string getUUIDStr();

  static std::size_t getBlockSize() {
    static std::size_t k_i{64 * 1024};
    return k_i;
  };

  std::string get_server_host();

  void from_json(const nlohmann::json& nlohmann_json_j);

  /**
   * @brief 全局是否停止， 服务器使用
   */
  bool p_stop;
  /**
   * @brief 全局锁,服务器使用
   */
  std::mutex p_mutex;
  /**
   * @brief 全局条件变量,服务器使用
   */
  std::condition_variable p_condition;

 private:
  /**
   * @brief 在初始化的时候，我们会进行一些设置，这些设置是及其基本的
   *
   */
  CoreSet();
  //获得缓存磁盘路径
  void getCacheDiskPath();
  //获得本地的有限设置
  void getSetting();

  static std::string configFileName();

 private:
  boost::uuids::random_generator p_uuid_gen;

  //用户名称
  std::string p_user_;
  //部门
  Department p_department_;

  FSys::path p_cache_root;
  FSys::path p_doc;
  FSys::path p_data_root;

  Ue4Setting &p_ue4_setting;

  FSys::path p_mayaPath;
  std::string p_server_host;  ///< 我们自己的服务器ip

  int p_sql_port;       ///< mysql 端口
  int p_meta_rpc_port;  ///< 元数据端口
  int p_file_rpc_port;  ///< filesys 文件传输端口

  std::string p_sql_host;      ///< mysql数据库ip
  std::string p_sql_user;      ///< mysql 用户名称
  std::string p_sql_password;  ///< mysql 用户密码

  //这里是序列化的代码
  friend class cereal::access;
  template <class Archive>
  void serialize(Archive &ar, std::uint32_t const version);
};

template <class Archive>
void CoreSet::serialize(Archive &ar, std::uint32_t const version) {
  if (version == 7)
    ar(
        cereal::make_nvp("user", p_user_),
        cereal::make_nvp("department", p_department_),
        cereal::make_nvp("ue4_setting", p_ue4_setting),
        cereal::make_nvp("maya_Path", p_mayaPath));
}

}  // namespace doodle
namespace cereal {
template <class Archive>
std::string save_minimal(Archive const &, doodle::Department const &department) {
  return std::string{magic_enum::enum_name(department)};
}
template <class Archive>
void load_minimal(Archive const &, doodle::Department &department, std::string const &value) {
  department = magic_enum::enum_cast<doodle::Department>(value).value_or(doodle::Department::None_);
};
}  // namespace cereal
CEREAL_CLASS_VERSION(doodle::CoreSet, 7);

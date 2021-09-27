#pragma once

#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Metadata/project.h>
#include <DoodleLib/core/Ue4Setting.h>
#include <DoodleLib/core/observable_container.h>
#include <DoodleLib/libWarp/boost_uuid_warp.h>

#include <boost/filesystem.hpp>
#include <magic_enum.hpp>
#include <nlohmann/json_fwd.hpp>
namespace doodle {

enum class department {
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

class DOODLELIB_API core_set : public details::no_copy {
 public:
  static core_set &getSet();

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
  [[nodiscard]] const department &getDepartmentEnum() const;
  void setDepartment(const std::string &value);
  void setDepartment(const department &value);

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

  void from_json(const nlohmann::json &nlohmann_json_j);

  std::uint16_t p_max_thread;
  void set_max_tread(const std::uint16_t in);
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
  core_set();
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
  department p_department_;

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
  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive &ar, std::uint32_t const version);
};

template <class Archive>
void core_set::serialize(Archive &ar, std::uint32_t const version) {
  if (version == 7)
    ar &
            boost::serialization::make_nvp("user", p_user_) &
        boost::serialization::make_nvp("department", p_department_) &
        boost::serialization::make_nvp("ue4_setting", p_ue4_setting) &
        boost::serialization::make_nvp("maya_Path", p_mayaPath);
  if (version == 8)
    ar &
            boost::serialization::make_nvp("user", p_user_) &
        boost::serialization::make_nvp("department", p_department_) &
        boost::serialization::make_nvp("ue4_setting", p_ue4_setting) &
        boost::serialization::make_nvp("maya_Path", p_mayaPath) &
        boost::serialization::make_nvp("p_max_thread", p_max_thread);
}

}  // namespace doodle
namespace cereal {
template <class Archive>
std::string save_minimal(Archive const &, doodle::department const &department) {
  return std::string{magic_enum::enum_name(department)};
}
template <class Archive>
void load_minimal(Archive const &, doodle::department &department, std::string const &value) {
  department = magic_enum::enum_cast<doodle::department>(value).value_or(doodle::department::None_);
};
}  // namespace cereal
BOOST_CLASS_VERSION(doodle::core_set, 8);
BOOST_CLASS_EXPORT_KEY(doodle::core_set);
